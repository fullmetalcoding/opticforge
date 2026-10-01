// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "PsfRenderController.h"

#include <chrono>
#include <exception>

namespace opticforge::renderer
{
    PsfRenderController::~PsfRenderController()
    {
        shutdown();
    }

    void PsfRenderController::shutdown() noexcept
    {
        m_pending.reset();

        if (m_cancel)
            m_cancel->store(true, std::memory_order_relaxed);

        if (m_job.valid())
        {
            try
            {
                m_job.wait();
                (void)m_job.get();
            }
            catch (...)
            {
            }
        }

        m_cancel.reset();
    }

    bool PsfRenderController::canReuseAccumulation(
        const Request& r) const noexcept
    {
        return
            m_accumulation &&
            m_accumulationTraceVersion == r.traceVersion &&
            psfAccumulationSettingsEqual(
                m_accumulationSettings,
                r.settings);
    }

    void PsfRenderController::request(
        std::shared_ptr<const raytracer::CompletedTrace> trace,
        std::uint64_t traceVersion,
        const PsfRenderSettings& settings)
    {
        if (!trace)
            return;

        Request r{ std::move(trace), traceVersion, settings };

        if (!m_job.valid())
        {
            start(std::move(r));
            return;
        }

        // A job is running. If the new request needs a different
        // accumulation, the running job's work is wasted: cancel it.
        // Otherwise (tonemap-only change) let it finish; the queued
        // request will reuse its accumulation.
        const bool sameAccumulation =
            m_running.traceVersion == r.traceVersion &&
            psfAccumulationSettingsEqual(
                m_running.settings,
                r.settings);

        if (!sameAccumulation && m_cancel)
            m_cancel->store(true, std::memory_order_relaxed);

        m_pending = std::move(r);
    }

    void PsfRenderController::start(Request r)
    {
        auto cancel = std::make_shared<std::atomic<bool>>(false);

        // Reuse the cached accumulation when only tonemap settings changed.
        std::shared_ptr<const PsfAccumulation> cached =
            canReuseAccumulation(r)
            ? m_accumulation
            : nullptr;

        // Move the trace into the job: m_running keeps only the version
        // and settings, so the worker holds the last reference we own.
        auto trace = std::move(r.trace);
        const PsfRenderSettings settings = r.settings;

        m_job = std::async(
            std::launch::async,
            [trace = std::move(trace), settings, cancel, cached]() mutable
            -> Outcome
            {
                // std::async keeps the callable alive until the future is
                // released on the main thread, so take the trace out of
                // the capture: it is then dropped here, on the worker.
                const auto job = std::move(trace);

                Outcome outcome;

                try
                {
                    PsfExecution execution;
                    execution.cancel = cancel.get();

                    auto accumulation = cached;

                    if (!accumulation)
                    {
                        accumulation =
                            std::make_shared<const PsfAccumulation>(
                                accumulatePsf(
                                    job->result,
                                    job->observationPlane,
                                    settings,
                                    {},
                                    execution));
                    }

                    outcome.image =
                        tonemapPsf(*accumulation, settings, execution);

                    outcome.accumulation = std::move(accumulation);
                }
                catch (const PsfCancelled&)
                {
                    outcome.cancelled = true;
                }
                catch (const std::exception& e)
                {
                    outcome.error = e.what();
                }
                catch (...)
                {
                    outcome.error = "Unknown error while rendering the PSF.";
                }

                // `job` is released here, on the worker thread. If the
                // controller has already published a newer trace, the old
                // TraceResult (potentially millions of allocations) is
                // freed off the UI thread.
                return outcome;
            });

        m_cancel = std::move(cancel);
        m_running = std::move(r);
    }

    std::optional<PsfImage> PsfRenderController::poll()
    {
        std::optional<PsfImage> ready;

        if (
            m_job.valid() &&
            m_job.wait_for(std::chrono::seconds(0)) ==
            std::future_status::ready)
        {
            Outcome outcome = m_job.get();

            m_cancel.reset();

            if (!outcome.cancelled)
            {
                if (!outcome.error.empty())
                {
                    m_error = std::move(outcome.error);
                }
                else
                {
                    m_error.clear();

                    m_accumulation = std::move(outcome.accumulation);
                    m_accumulationTraceVersion = m_running.traceVersion;
                    m_accumulationSettings = m_running.settings;

                    // Shown even if a newer request is queued: during a
                    // slider drag the view keeps updating at job rate.
                    ready = std::move(outcome.image);
                }
            }

            m_running = {};
        }

        if (!m_job.valid() && m_pending)
        {
            Request next = std::move(*m_pending);
            m_pending.reset();
            start(std::move(next));
        }

        return ready;
    }
}
