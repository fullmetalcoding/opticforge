// TraceController.cpp
#include "TraceController.h"
#include "Raytracer.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iterator>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace opticforge::raytracer
{
    namespace
    {
        constexpr double TwoPi = 6.28318530717958647692;
        constexpr std::size_t BatchSize = 256;

        bool finiteVector(const glm::dvec3& value)
        {
            return std::isfinite(value.x)
                && std::isfinite(value.y)
                && std::isfinite(value.z);
        }

        bool positiveFinite(double value)
        {
            return std::isfinite(value) && value > 0.0;
        }

        void validateAperture(const optics::Aperture& aperture)
        {
            std::visit(
                [](const auto& shape)
                {
                    using T = std::decay_t<decltype(shape)>;

                    if constexpr (
                        std::is_same_v<T, optics::CircularAperture>)
                    {
                        if (!positiveFinite(shape.radius))
                        {
                            throw std::invalid_argument(
                                "Launch pupil radius must be positive.");
                        }
                    }
                    else if constexpr (
                        std::is_same_v<T, optics::AnnularAperture>)
                    {
                        if (!std::isfinite(shape.innerRadius)
                            || shape.innerRadius < 0.0
                            || !positiveFinite(shape.outerRadius)
                            || shape.innerRadius >= shape.outerRadius)
                        {
                            throw std::invalid_argument(
                                "Invalid annular launch pupil radii.");
                        }
                    }
                    else
                    {
                        static_assert(
                            std::is_same_v<
                            T, optics::RectangularAperture>,
                            "Unsupported launch aperture");

                        if (!positiveFinite(shape.width)
                            || !positiveFinite(shape.height))
                        {
                            throw std::invalid_argument(
                                "Launch pupil dimensions must be positive.");
                        }
                    }
                },
                aperture.geometry());
        }

        // Uniform-area sampling. The caller validates the aperture first.
        glm::dvec2 sampleAperture(
            const optics::Aperture& aperture,
            double u,
            double v)
        {
            return std::visit(
                [u, v](const auto& shape) -> glm::dvec2
                {
                    using T = std::decay_t<decltype(shape)>;

                    if constexpr (
                        std::is_same_v<T, optics::RectangularAperture>)
                    {
                        return {
                            (u - 0.5) * shape.width,
                            (v - 0.5) * shape.height
                        };
                    }
                    else
                    {
                        double radius;

                        if constexpr (
                            std::is_same_v<T, optics::CircularAperture>)
                        {
                            radius = shape.radius * std::sqrt(u);
                        }
                        else
                        {
                            static_assert(
                                std::is_same_v<
                                T, optics::AnnularAperture>,
                                "Unsupported launch aperture");

                            const double ratio =
                                shape.innerRadius / shape.outerRadius;

                            radius = shape.outerRadius * std::sqrt(
                                ratio * ratio
                                + u * (1.0 - ratio * ratio));
                        }

                        const double angle = TwoPi * v;

                        return {
                            radius * std::cos(angle),
                            radius * std::sin(angle)
                        };
                    }
                },
                aperture.geometry());
        }
    }

    TraceController::~TraceController()
    {
        if (m_control)
        {
            m_control->cancelRequested.store(
                true, std::memory_order_relaxed);
        }

        // Ensure the worker finishes before controller destruction.
        // The worker owns its snapshot and never accesses this object.
        if (m_job.valid())
        {
            m_job.wait();
        }
    }

    void TraceController::invalidate()
    {
        ++m_revision;
        m_dirty = true;
        m_lastChange = Clock::now();

        // Preserve explicit pending requests. Automatic replacements
        // are scheduled by update() once editing has paused.
        if (isRunning() && m_control)
        {
            m_control->cancelRequested.store(
                true, std::memory_order_relaxed);

            m_status = TraceStatus::Cancelling;
        }
    }

    void TraceController::requestTrace()
    {
        // Also invalidates any currently running result, even if callers
        // request a run without separately calling invalidate().
        invalidate();
        m_traceRequested = true;
    }

    void TraceController::requestCancel()
    {
        m_traceRequested = false;

        // Suppress automatic restart until another edit, explicit
        // request, or re-enabling of a view/auto-update.
        m_dirty = false;

        if (isRunning() && m_control)
        {
            m_control->cancelRequested.store(
                true, std::memory_order_relaxed);

            m_status = TraceStatus::Cancelling;
        }
    }

    void TraceController::setAutoUpdate(bool enabled)
    {
        if (m_autoUpdate == enabled)
            return;

        m_autoUpdate = enabled;

        if (enabled && !resultIsCurrent())
        {
            m_dirty = true;
            m_lastChange = Clock::now();
        }
    }

    void TraceController::setResultsNeeded(bool needed)
    {
        if (m_resultsNeeded == needed)
            return;

        m_resultsNeeded = needed;

        if (needed && !resultIsCurrent())
        {
            m_dirty = true;

            // Opening a view can start immediately.
            m_lastChange = Clock::now() - m_debounceDelay;
        }

        // Closing both views prevents further automatic runs.
        // An already running job is allowed to finish and be cached.
    }

    double TraceController::progress() const noexcept
    {
        if (!isRunning())
        {
            return m_status == TraceStatus::Completed ? 1.0 : 0.0;
        }

        if (!m_control || m_activeRayCount == 0)
            return 0.0;

        const std::size_t completed =
            m_control->completedRays.load(std::memory_order_relaxed);

        return std::clamp(
            static_cast<double>(completed)
            / static_cast<double>(m_activeRayCount),
            0.0,
            1.0);
    }

    void TraceController::update(
        const telescope::TelescopeProject& project,
        const TraceSettings& settings)
    {
        pollJob();

        if (m_job.valid())
            return;

        const bool autoReady =
            m_autoUpdate
            && m_resultsNeeded
            && m_dirty
            && Clock::now() - m_lastChange >= m_debounceDelay;

        if (m_traceRequested || autoReady)
        {
            startJob(project, settings);
        }
    }

    void TraceController::startJob(
        const telescope::TelescopeProject& project,
        const TraceSettings& settings)
    {
        // Consume the request before starting. Failure does not trigger
        // an automatic retry every frame.
        m_traceRequested = false;
        m_dirty = false;
        m_errorMessage.clear();

        try
        {
            TraceSnapshot snapshot;
            snapshot.primitives = project.primitives();
            snapshot.launchPupil = project.getLaunchPupil();
            snapshot.observationPlane = project.getObservationPlane();
            snapshot.settings = settings;
            snapshot.revision = m_revision;

            auto control = std::make_shared<TraceControl>();

            auto job = std::async(
                std::launch::async,
                [snapshot = std::move(snapshot), control]() mutable
                {
                    return TraceController::runJob(
                        std::move(snapshot), control);
                });

            m_control = std::move(control);
            m_job = std::move(job);
            m_activeRayCount = settings.rayCount;
            m_status = TraceStatus::Running;
        }
        catch (const std::exception& error)
        {
            m_control.reset();
            m_activeRayCount = 0;
            m_status = TraceStatus::Failed;
            m_errorMessage = error.what();
        }
        catch (...)
        {
            m_control.reset();
            m_activeRayCount = 0;
            m_status = TraceStatus::Failed;
            m_errorMessage = "Unable to start the trace job.";
        }
    }

    void TraceController::pollJob()
    {
        if (!m_job.valid())
            return;

        if (m_job.wait_for(std::chrono::milliseconds{ 0 })
            != std::future_status::ready)
        {
            return;
        }

        const bool cancelRequested =
            m_control
            && m_control->cancelRequested.load(
                std::memory_order_relaxed);

        try
        {
            JobOutcome outcome = m_job.get();

            if (cancelRequested || outcome.cancelled)
            {
                m_status = TraceStatus::Cancelled;
            }
            else if (!outcome.error.empty())
            {
                m_status = TraceStatus::Failed;
                m_errorMessage = std::move(outcome.error);
            }
            else if (!outcome.completed)
            {
                m_status = TraceStatus::Failed;
                m_errorMessage = "Trace job returned no result.";
            }
            else if (outcome.completed->revision != m_revision)
            {
                // A newer edit superseded this job.
                // Keep the previous published result.
                m_status = TraceStatus::Cancelled;
            }
            else
            {
                m_latestResult = std::move(outcome.completed);
                ++m_resultVersion;

                m_status = TraceStatus::Completed;
                m_errorMessage.clear();
            }
        }
        catch (const std::exception& error)
        {
            m_status = TraceStatus::Failed;
            m_errorMessage = error.what();
        }
        catch (...)
        {
            m_status = TraceStatus::Failed;
            m_errorMessage = "Unknown error while completing the trace.";
        }

        m_control.reset();
        m_activeRayCount = 0;
    }

    TraceController::JobOutcome TraceController::runJob(
        TraceSnapshot snapshot,
        std::shared_ptr<TraceControl> control)
    {
        JobOutcome outcome;

        try
        {
            const auto cancelled = [&]()
                {
                    return control->cancelRequested.load(
                        std::memory_order_relaxed);
                };

            if (cancelled())
            {
                outcome.cancelled = true;
                return outcome;
            }

            const auto& settings = snapshot.settings;
            const auto& pupil = snapshot.launchPupil;

            if (settings.rayCount == 0)
                throw std::invalid_argument("Ray count must be positive.");

            if (settings.maxInteractions == 0)
            {
                throw std::invalid_argument(
                    "Maximum interactions must be positive.");
            }

            if (!positiveFinite(settings.wavelengthNm))
            {
                throw std::invalid_argument(
                    "Wavelength must be finite and positive.");
            }

            validateAperture(pupil.aperture);

            const double directionLength =
                glm::length(pupil.localDirection);

            if (!finiteVector(pupil.localDirection)
                || !positiveFinite(directionLength))
            {
                throw std::invalid_argument(
                    "Invalid launch pupil direction.");
            }

            const glm::dvec3 direction =
                pupil.transform.localToWorldDirection(
                    pupil.localDirection / directionLength);

            if (!finiteVector(direction)
                || !positiveFinite(glm::length(direction)))
            {
                throw std::invalid_argument(
                    "Invalid transformed launch direction.");
            }

            std::mt19937_64 rng(settings.randomSeed);
            std::uniform_real_distribution<double> uniform(0.0, 1.0);

            RayTracer tracer;

            CompletedTrace completed;
            completed.observationPlane = snapshot.observationPlane;
            completed.settings = settings;
            completed.revision = snapshot.revision;
            completed.result.paths.reserve(settings.rayCount);

            std::vector<optics::OpticalRay> batch;
            batch.reserve(std::min(BatchSize, settings.rayCount));

            std::size_t traced = 0;

            while (traced < settings.rayCount)
            {
                if (cancelled())
                {
                    outcome.cancelled = true;
                    return outcome;
                }

                const std::size_t count = std::min(
                    BatchSize, settings.rayCount - traced);

                batch.clear();

                for (std::size_t i = 0; i < count; ++i)
                {
                    // Separate calls preserve a defined sampling order.
                    const double u = uniform(rng);
                    const double v = uniform(rng);

                    const glm::dvec2 sample =
                        sampleAperture(pupil.aperture, u, v);

                    const glm::dvec3 origin =
                        pupil.transform.localToWorldPoint(
                            glm::dvec3(sample.x, sample.y, 0.0));

                    if (!finiteVector(origin))
                    {
                        throw std::invalid_argument(
                            "Invalid transformed launch position.");
                    }

                    optics::OpticalRay ray;
                    ray.ray = optics::Ray(origin, direction);
                    ray.wavelength = settings.wavelengthNm;
                    ray.intensity = 1.0;

                    batch.push_back(ray);
                }

                if (cancelled())
                {
                    outcome.cancelled = true;
                    return outcome;
                }

                TraceResult batchResult = tracer.traceRayBundle(
                    batch,
                    snapshot.primitives,
                    snapshot.observationPlane,
                    settings.maxInteractions);

                if (cancelled())
                {
                    outcome.cancelled = true;
                    return outcome;
                }

                if (batchResult.paths.size() != count)
                {
                    throw std::runtime_error(
                        "Tracer returned an unexpected number of paths.");
                }

                completed.result.paths.insert(
                    completed.result.paths.end(),
                    std::make_move_iterator(batchResult.paths.begin()),
                    std::make_move_iterator(batchResult.paths.end()));

                traced += count;

                control->completedRays.store(
                    traced, std::memory_order_relaxed);
            }

            if (cancelled())
            {
                outcome.cancelled = true;
                return outcome;
            }

            outcome.completed = std::move(completed);
        }
        catch (const std::exception& error)
        {
            outcome.error = error.what();
        }
        catch (...)
        {
            outcome.error = "Unknown error during ray tracing.";
        }

        return outcome;
    }
}