// TraceController.cpp

#include "TraceController.h"
#include "Raytracer.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <exception>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>


namespace opticforge::raytracer
{
    namespace
    {
        constexpr double TwoPi =
            6.28318530717958647692;


        // Workers claim this many rays at a time.
        //
        // This is not a synchronization barrier like the old BatchSize.
        // Every worker independently asks for another chunk as soon as it
        // finishes its current one.
        constexpr std::size_t WorkChunkSize = 32;


        bool finiteVector(const glm::dvec3& value)
        {
            return
                std::isfinite(value.x) &&
                std::isfinite(value.y) &&
                std::isfinite(value.z);
        }


        bool positiveFinite(double value)
        {
            return
                std::isfinite(value) &&
                value > 0.0;
        }


        void validateAperture(
            const optics::Aperture& aperture)
        {
            std::visit(
                [](const auto& shape)
                {
                    using T =
                        std::decay_t<decltype(shape)>;


                    if constexpr (
                        std::is_same_v<
                        T,
                        optics::CircularAperture>)
                    {
                        if (!positiveFinite(shape.radius))
                        {
                            throw std::invalid_argument(
                                "Launch pupil radius must be positive.");
                        }
                    }
                    else if constexpr (
                        std::is_same_v<
                        T,
                        optics::AnnularAperture>)
                    {
                        if (
                            !std::isfinite(shape.innerRadius) ||
                            shape.innerRadius < 0.0 ||
                            !positiveFinite(shape.outerRadius) ||
                            shape.innerRadius >= shape.outerRadius)
                        {
                            throw std::invalid_argument(
                                "Invalid annular launch pupil radii.");
                        }
                    }
                    else if constexpr(
                        std::is_same_v<
                        T,
                        optics::EllipticalAperture>)
                    {
                        //FIll me in with validation code
                    }
                    else
                    {
                        static_assert(
                            std::is_same_v<
                            T,
                            optics::RectangularAperture>,
                            "Unsupported launch aperture");

                        if (
                            !positiveFinite(shape.width) ||
                            !positiveFinite(shape.height))
                        {
                            throw std::invalid_argument(
                                "Launch pupil dimensions must be positive.");
                        }
                    }
                },
                aperture.geometry());
        }


        // Uniform-area launch-pupil sampling.
        //
        // The caller validates the aperture before using this function.
        glm::dvec2 sampleAperture(
            const optics::Aperture& aperture,
            double u,
            double v)
        {
            return std::visit(
                [u, v](const auto& shape) -> glm::dvec2
                {
                    using T =
                        std::decay_t<decltype(shape)>;


                    if constexpr (
                        std::is_same_v<
                        T,
                        optics::RectangularAperture>)
                    {
                        return
                        {
                            (u - 0.5) * shape.width,
                            (v - 0.5) * shape.height
                        };
                    }
                    else if constexpr (
                        std::is_same_v<
                        T,
                        optics::EllipticalAperture>)
                    {
                        //@TODO: Fill me in
                        return { 0,0 };
                    }
                    else
                    {
                        double radius = 0.0;


                        if constexpr (
                            std::is_same_v<
                            T,
                            optics::CircularAperture>)
                        {
                            radius =
                                shape.radius *
                                std::sqrt(u);
                        }
                        else
                        {
                            static_assert(
                                std::is_same_v<
                                T,
                                optics::AnnularAperture>,
                                "Unsupported launch aperture");

                            const double ratio =
                                shape.innerRadius /
                                shape.outerRadius;

                            radius =
                                shape.outerRadius *
                                std::sqrt(
                                    ratio * ratio +
                                    u * (
                                        1.0 -
                                        ratio * ratio));
                        }


                        const double angle =
                            TwoPi * v;


                        return
                        {
                            radius * std::cos(angle),
                            radius * std::sin(angle)
                        };
                    }
                },
                aperture.geometry());
        }


        unsigned int resolveWorkerCount(
            const TraceSettings& settings)
        {
            const unsigned int hardware =
                std::max(
                    1u,
                    std::thread::hardware_concurrency());


            unsigned int workers =
                settings.workerCount;


            if (workers == 0)
            {
                // Automatic mode intentionally leaves one logical CPU
                // available for the main/render thread when possible.
                workers =
                    hardware > 1
                    ? hardware - 1
                    : 1;
            }


            workers =
                std::clamp(
                    workers,
                    1u,
                    hardware);


            // There is no reason to create more workers than rays.
            if (settings.rayCount <
                static_cast<std::size_t>(workers))
            {
                workers =
                    static_cast<unsigned int>(
                        settings.rayCount);
            }


            return std::max(1u, workers);
        }
    }


    TraceController::~TraceController()
    {
        if (m_control)
        {
            m_control->cancelRequested.store(
                true,
                std::memory_order_relaxed);
        }


        // The async job owns the snapshot and all worker threads.
        //
        // Do not destroy the controller until they have all terminated.
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


        // Preserve an explicit requestTrace() request if one already exists.
        // update() will start the replacement after the old job exits.
        if (isRunning() && m_control)
        {
            m_control->cancelRequested.store(
                true,
                std::memory_order_relaxed);

            m_status =
                TraceStatus::Cancelling;
        }
    }


    void TraceController::requestTrace()
    {
        // Also makes any previously completed solution stale.
        invalidate();

        // Explicit requests bypass the debounce delay.
        m_traceRequested = true;
    }


    void TraceController::requestCancel()
    {
        m_traceRequested = false;

        // Suppress automatic restart until another edit/request occurs.
        m_dirty = false;


        if (isRunning() && m_control)
        {
            m_control->cancelRequested.store(
                true,
                std::memory_order_relaxed);

            m_status =
                TraceStatus::Cancelling;
        }
    }


    void TraceController::setAutoUpdate(bool enabled)
    {
        if (m_autoUpdate == enabled)
            return;


        m_autoUpdate = enabled;


        if (
            enabled &&
            !resultIsCurrent())
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


        if (
            needed &&
            !resultIsCurrent())
        {
            m_dirty = true;

            // Opening a result view should not incur the normal edit
            // debounce delay.
            m_lastChange =
                Clock::now() -
                m_debounceDelay;
        }


        // If both result views close while a trace is already running, allow
        // that trace to finish and cache its result.
    }


    double TraceController::progress() const noexcept
    {
        if (!isRunning())
        {
            return
                m_status == TraceStatus::Completed
                ? 1.0
                : 0.0;
        }


        if (
            !m_control ||
            m_activeRayCount == 0)
        {
            return 0.0;
        }


        const std::size_t completed =
            m_control->completedRays.load(
                std::memory_order_relaxed);


        return std::clamp(
            static_cast<double>(completed) /
            static_cast<double>(m_activeRayCount),
            0.0,
            1.0);
    }


    void TraceController::update(
        const telescope::TelescopeProject& project,
        const TraceSettings& settings)
    {
        pollJob();


        // pollJob() invalidates the future by calling get() when the
        // background job is complete.
        //
        // Until that happens, do not start another job.
        if (m_job.valid())
            return;


        const bool autoReady =
            m_autoUpdate &&
            m_resultsNeeded &&
            m_dirty &&
            Clock::now() - m_lastChange >=
            m_debounceDelay;


        if (
            m_traceRequested ||
            autoReady)
        {
            startJob(
                project,
                settings);
        }
    }


    void TraceController::startJob(
        const telescope::TelescopeProject& project,
        const TraceSettings& settings)
    {
        // Consume the pending request before attempting to start.
        //
        // If startup itself fails we don't want update() retrying every
        // single frame.
        m_traceRequested = false;
        m_dirty = false;

        m_errorMessage.clear();


        try
        {
            TraceSnapshot snapshot;

            snapshot.primitives =
                project.primitives();

            snapshot.launchPupil =
                project.getLaunchPupil();

            snapshot.observationPlane =
                project.getObservationPlane();

            snapshot.settings =
                settings;

            snapshot.revision =
                m_revision;


            auto control =
                std::make_shared<TraceControl>();


            auto job =
                std::async(
                    std::launch::async,

                    [
                        snapshot = std::move(snapshot),
                        control
                    ]() mutable
                    {
                        return
                            TraceController::runJob(
                                std::move(snapshot),
                                control);
                    });


            m_control =
                std::move(control);

            m_job =
                std::move(job);

            m_activeRayCount =
                settings.rayCount;

            m_status =
                TraceStatus::Running;
        }
        catch (const std::exception& error)
        {
            m_control.reset();

            m_activeRayCount = 0;

            m_status =
                TraceStatus::Failed;

            m_errorMessage =
                error.what();
        }
        catch (...)
        {
            m_control.reset();

            m_activeRayCount = 0;

            m_status =
                TraceStatus::Failed;

            m_errorMessage =
                "Unable to start the trace job.";
        }
    }


    void TraceController::pollJob()
    {
        if (!m_job.valid())
            return;


        if (
            m_job.wait_for(
                std::chrono::milliseconds{ 0 }) !=
            std::future_status::ready)
        {
            return;
        }


        const bool cancelRequested =
            m_control &&
            m_control->cancelRequested.load(
                std::memory_order_relaxed);


        try
        {
            JobOutcome outcome =
                m_job.get();


            if (
                cancelRequested ||
                outcome.cancelled)
            {
                m_status =
                    TraceStatus::Cancelled;
            }
            else if (!outcome.error.empty())
            {
                m_status =
                    TraceStatus::Failed;

                m_errorMessage =
                    std::move(outcome.error);
            }
            else if (!outcome.completed)
            {
                m_status =
                    TraceStatus::Failed;

                m_errorMessage =
                    "Trace job returned no result.";
            }
            else if (
                outcome.completed->revision !=
                m_revision)
            {
                // Some newer project/settings revision superseded this job.
                //
                // Keep displaying the previous published solution.
                m_status =
                    TraceStatus::Cancelled;
            }
            else
            {
                m_latestResult =
                    std::move(outcome.completed);

                ++m_resultVersion;

                m_status =
                    TraceStatus::Completed;

                m_errorMessage.clear();
            }
        }
        catch (const std::exception& error)
        {
            m_status =
                TraceStatus::Failed;

            m_errorMessage =
                error.what();
        }
        catch (...)
        {
            m_status =
                TraceStatus::Failed;

            m_errorMessage =
                "Unknown error while completing the trace.";
        }


        m_control.reset();

        m_activeRayCount = 0;
    }


    TraceController::JobOutcome
        TraceController::runJob(
            TraceSnapshot snapshot,
            std::shared_ptr<TraceControl> control)
    {
        JobOutcome outcome;


        try
        {
            const auto cancelled =
                [&]()
                {
                    return
                        control->cancelRequested.load(
                            std::memory_order_relaxed);
                };


            if (cancelled())
            {
                outcome.cancelled = true;
                return outcome;
            }


            const auto& settings =
                snapshot.settings;

            const auto& pupil =
                snapshot.launchPupil;


            //
            // ------------------------------------------------------------
            // Validate trace settings
            // ------------------------------------------------------------
            //

            if (settings.rayCount == 0)
            {
                throw std::invalid_argument(
                    "Ray count must be positive.");
            }


            if (settings.maxInteractions == 0)
            {
                throw std::invalid_argument(
                    "Maximum interactions must be positive.");
            }


            if (!positiveFinite(
                settings.wavelengthNm))
            {
                throw std::invalid_argument(
                    "Wavelength must be finite and positive.");
            }


            validateAperture(
                pupil.aperture);


            const double directionLength =
                glm::length(
                    pupil.localDirection);


            if (
                !finiteVector(
                    pupil.localDirection) ||
                !positiveFinite(
                    directionLength))
            {
                throw std::invalid_argument(
                    "Invalid launch pupil direction.");
            }


            const glm::dvec3 direction =
                pupil.transform.localToWorldDirection(
                    pupil.localDirection /
                    directionLength);


            if (
                !finiteVector(direction) ||
                !positiveFinite(
                    glm::length(direction)))
            {
                throw std::invalid_argument(
                    "Invalid transformed launch direction.");
            }


            //
            // ------------------------------------------------------------
            // Generate the launch bundle
            // ------------------------------------------------------------
            //
            // Keep bundle generation single-threaded.
            //
            // This preserves deterministic correspondence between
            // randomSeed and the generated ray sequence regardless of the
            // requested worker count.
            //

            std::mt19937_64 rng(
                settings.randomSeed);

            std::uniform_real_distribution<double>
                uniform(
                    0.0,
                    1.0);


            std::vector<optics::OpticalRay> rays;

            rays.reserve(
                settings.rayCount);


            for (
                std::size_t i = 0;
                i < settings.rayCount;
                ++i)
            {
                if (cancelled())
                {
                    outcome.cancelled = true;
                    return outcome;
                }


                // Separate RNG calls preserve the exact sampling order.
                const double u =
                    uniform(rng);

                const double v =
                    uniform(rng);


                const glm::dvec2 sample =
                    sampleAperture(
                        pupil.aperture,
                        u,
                        v);


                const glm::dvec3 origin =
                    pupil.transform.localToWorldPoint(
                        glm::dvec3(
                            sample.x,
                            sample.y,
                            0.0));


                if (!finiteVector(origin))
                {
                    throw std::invalid_argument(
                        "Invalid transformed launch position.");
                }


                optics::OpticalRay ray;

                ray.ray =
                    optics::Ray(
                        origin,
                        direction);

                ray.wavelength =
                    settings.wavelengthNm;

                ray.intensity =
                    1.0;


                rays.push_back(
                    std::move(ray));
            }


            if (cancelled())
            {
                outcome.cancelled = true;
                return outcome;
            }


            //
            // ------------------------------------------------------------
            // Prepare the completed result
            // ------------------------------------------------------------
            //

            CompletedTrace completed;

            completed.observationPlane =
                snapshot.observationPlane;

            completed.settings =
                settings;

            completed.revision =
                snapshot.revision;


            // Critical:
            //
            // Allocate all result slots before starting workers. Each worker
            // writes only to uniquely owned indices, so no mutex is required
            // around result.paths.
            completed.result.paths.resize(
                settings.rayCount);


            //
            // ------------------------------------------------------------
            // Resolve execution policy
            // ------------------------------------------------------------
            //

            const unsigned int workerCount =
                resolveWorkerCount(settings);


            // nextRay is the dynamic work queue.
            //
            // A worker reserves WorkChunkSize consecutive indices at a time.
            // There is no global batch barrier.
            std::atomic<std::size_t> nextRay{ 0 };


            // Used for failures internal to workers.
            //
            // Do NOT set cancelRequested for an internal exception, because
            // pollJob() intentionally treats user/project cancellation
            // differently from an actual trace failure.
            std::atomic<bool> stopWorkers{ false };

            std::mutex exceptionMutex;

            std::exception_ptr workerException;


            const auto recordWorkerException =
                [&]()
                {
                    {
                        std::lock_guard<std::mutex>
                            lock(exceptionMutex);

                        if (!workerException)
                        {
                            workerException =
                                std::current_exception();
                        }
                    }


                    stopWorkers.store(
                        true,
                        std::memory_order_relaxed);
                };


            //
            // ------------------------------------------------------------
            // Worker function
            // ------------------------------------------------------------
            //

            const auto worker =
                [&]()
                {
                    try
                    {
                        // RayTracer is currently stateless, but giving each
                        // worker its own instance avoids introducing a hidden
                        // shared-state dependency if that changes later.
                        RayTracer tracer;


                        while (true)
                        {
                            if (
                                stopWorkers.load(
                                    std::memory_order_relaxed) ||
                                cancelled())
                            {
                                return;
                            }


                            const std::size_t begin =
                                nextRay.fetch_add(
                                    WorkChunkSize,
                                    std::memory_order_relaxed);


                            if (begin >= rays.size())
                                return;


                            const std::size_t end =
                                std::min(
                                    begin + WorkChunkSize,
                                    rays.size());


                            std::size_t completedInChunk = 0;


                            for (
                                std::size_t i = begin;
                                i < end;
                                ++i)
                            {
                                if (
                                    stopWorkers.load(
                                        std::memory_order_relaxed) ||
                                    cancelled())
                                {
                                    break;
                                }


                                completed.result.paths[i] =
                                    tracer.traceRay(
                                        rays[i],
                                        snapshot.primitives,
                                        snapshot.observationPlane,
                                        settings.maxInteractions);


                                ++completedInChunk;
                            }


                            if (completedInChunk != 0)
                            {
                                control->completedRays.fetch_add(
                                    completedInChunk,
                                    std::memory_order_relaxed);
                            }


                            // Cancellation/failure might have occurred partway
                            // through this chunk.
                            if (
                                completedInChunk !=
                                end - begin)
                            {
                                return;
                            }
                        }
                    }
                    catch (...)
                    {
                        recordWorkerException();
                    }
                };


            //
            // ------------------------------------------------------------
            // Start worker threads
            // ------------------------------------------------------------
            //
            // runJob() itself is already executing on an asynchronous thread.
            //
            // Therefore:
            //
            // workerCount == 1:
            //     runJob thread does all tracing.
            //
            // workerCount == 10:
            //     create 9 additional threads and use the runJob thread as
            //     worker #10.
            //

            std::vector<std::thread> workers;

            if (workerCount > 1)
            {
                workers.reserve(
                    workerCount - 1);
            }


            try
            {
                for (
                    unsigned int i = 1;
                    i < workerCount;
                    ++i)
                {
                    workers.emplace_back(
                        worker);
                }
            }
            catch (...)
            {
                // Thread creation itself can fail. Existing std::threads must
                // be joined before their destructors run or std::terminate()
                // would be called.
                stopWorkers.store(
                    true,
                    std::memory_order_relaxed);


                for (auto& thread : workers)
                {
                    if (thread.joinable())
                        thread.join();
                }


                throw;
            }


            // The existing asynchronous job thread participates in the work
            // instead of sitting idle while child threads trace.
            worker();


            //
            // ------------------------------------------------------------
            // Join workers
            // ------------------------------------------------------------
            //

            for (auto& thread : workers)
            {
                if (thread.joinable())
                    thread.join();
            }


            //
            // ------------------------------------------------------------
            // Handle worker outcome
            // ------------------------------------------------------------
            //

            if (workerException)
            {
                std::rethrow_exception(
                    workerException);
            }


            if (cancelled())
            {
                outcome.cancelled = true;
                return outcome;
            }


            // If no cancellation or exception occurred, every ray must have
            // been completed.
            const std::size_t completedRayCount =
                control->completedRays.load(
                    std::memory_order_relaxed);


            if (
                completedRayCount !=
                settings.rayCount)
            {
                throw std::runtime_error(
                    "Ray trace terminated before all rays were completed.");
            }


            outcome.completed =
                std::move(completed);
        }
        catch (const std::exception& error)
        {
            outcome.error =
                error.what();
        }
        catch (...)
        {
            outcome.error =
                "Unknown error during ray tracing.";
        }


        return outcome;
    }
}