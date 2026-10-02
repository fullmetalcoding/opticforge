// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include "PsfRasterizer.h"
#include "raytracer/TraceController.h"

#include <atomic>
#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <string>

namespace opticforge::renderer
{
    // Runs PSF rasterization on a background thread so the UI/render
    // thread never waits for it.
    //
    // Mirrors TraceController's model:
    //   - the main thread calls request() whenever the trace or the PSF
    //     settings change, and poll() once per frame;
    //   - at most one job runs at a time; newer requests replace older
    //     pending ones ("latest wins"), so dragging a slider never
    //     builds a backlog;
    //   - a request that makes the running job obsolete cancels it.
    //
    // The expensive accumulation stage is cached. Changing only
    // background / exposure / normalizePeak re-runs the cheap tonemap
    // stage and skips re-splatting every ray.
    //
    // No OpenGL is touched here. Upload the PsfImage returned by poll()
    // with PsfTextureRenderer::upload() on the GL thread.
    class PsfRenderController
    {
    public:
        PsfRenderController() = default;
        ~PsfRenderController();

        PsfRenderController(const PsfRenderController&) = delete;
        PsfRenderController& operator=(const PsfRenderController&) = delete;

        // Main thread. traceVersion identifies the trace (normally
        // TraceController::resultVersion()).
        void request(
            std::shared_ptr<const raytracer::CompletedTrace> trace,
            std::uint64_t traceVersion,
            const PsfRenderSettings& settings);

        // Main thread, once per frame. Never blocks. Returns a finished
        // image at most once per completed job.
        std::optional<PsfImage> poll();

        // True while a job is running or queued.
        bool busy() const noexcept
        {
            return m_job.valid() || m_pending.has_value();
        }

        // Fraction [0, 1] of the running job that is complete, or 0 when
        // no job is running. Main thread; safe to call every frame.
        double progress() const noexcept
        {
            return m_progress
                ? m_progress->load(std::memory_order_relaxed)
                : 0.0;
        }

        const std::string& errorMessage() const noexcept
        {
            return m_error;
        }

        // Main thread. Cancels and waits for any running job.
        void shutdown() noexcept;

        // Test hook: blocks until the running job (if any) has finished,
        // without consuming its result. Lets tests issue a request after
        // the job's last cancellation check, deterministically.
        void waitForRunningJobForTesting() const
        {
            if (m_job.valid())
                m_job.wait();
        }

    private:
        struct Request
        {
            std::shared_ptr<const raytracer::CompletedTrace> trace;
            std::uint64_t traceVersion = 0;
            PsfRenderSettings settings;
        };

        struct Outcome
        {
            std::shared_ptr<const PsfAccumulation> accumulation;
            std::optional<PsfImage> image;
            bool cancelled = false;
            std::string error;
        };

        void start(Request request);

        bool canReuseAccumulation(const Request& r) const noexcept;

        std::future<Outcome> m_job;
        std::shared_ptr<std::atomic<bool>> m_cancel;
        std::shared_ptr<std::atomic<double>> m_progress;
        Request m_running;

        std::optional<Request> m_pending;

        // Cache of the last completed accumulation.
        std::shared_ptr<const PsfAccumulation> m_accumulation;
        std::uint64_t m_accumulationTraceVersion = 0;
        PsfRenderSettings m_accumulationSettings;

        std::string m_error;
    };
}
