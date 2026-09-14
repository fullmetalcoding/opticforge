// TraceController.h
#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "telescope/TelescopeProject.h"
#include "raytracer/TraceResult.h"

namespace opticforge::raytracer
{
    struct TraceSettings
    {
        std::size_t rayCount = 10000;
        std::uint32_t randomSeed = 1;
        std::uint32_t maxInteractions = 1000;

        double wavelengthNm = 550.0;
    };

    // Shared with workers. Contains no project or rendering data.
    struct TraceControl
    {
        std::atomic<bool> cancelRequested{ false };
        std::atomic<std::size_t> completedRays{ 0 };
    };

    enum class TraceStatus
    {
        Idle,
        Running,
        Cancelling,
        Completed,
        Cancelled,
        Failed
    };

    // Owned by the background job, keeping scene data stable.
    struct TraceSnapshot
    {
        std::vector<telescope::PrimitiveRecord> primitives;
        telescope::LaunchPupil launchPupil;
        telescope::ObservationPlane observationPlane;

        TraceSettings settings;

        // Identifies the optical configuration/settings used for this run.
        std::uint64_t revision = 0;
    };

    struct CompletedTrace
    {
        TraceResult result;

        // Preserve the coordinate frame used to interpret captured hits.
        telescope::ObservationPlane observationPlane;
        TraceSettings settings;
        std::uint64_t revision = 0;
    };

    class TraceController
    {
    public:
        TraceController() = default;
        ~TraceController();

        TraceController(const TraceController&) = delete;
        TraceController& operator=(const TraceController&) = delete;

        TraceController(TraceController&&) = delete;
        TraceController& operator=(TraceController&&) = delete;

        // Call when optical geometry or tracing settings change.
        // Does not copy the scene or start work.
        void invalidate();

        // Request an immediate trace, bypassing the debounce delay.
        // If a job is running, cancel it and schedule its replacement.
        void requestTrace();

        // Cancel the running job and any pending request.
        // Keep the last completed result.
        void requestCancel();

        void setAutoUpdate(bool enabled);

        // True while either ray paths or the PSF view needs results.
        void setResultsNeeded(bool needed);

        // Called once per frame on the main thread.
        //
        // 1. Poll an existing job without blocking.
        // 2. Publish a completed result or report failure/cancellation.
        // 3. Start a pending job when eligible.
        //
        // Copies project data only when starting a job.
        void update(
            const telescope::TelescopeProject& project,
            const TraceSettings& settings);

        TraceStatus status() const noexcept
        {
            return m_status;
        }

        bool isRunning() const noexcept
        {
            return m_status == TraceStatus::Running
                || m_status == TraceStatus::Cancelling;
        }

        bool autoUpdate() const noexcept
        {
            return m_autoUpdate;
        }

        // Fraction of rays completed in the active job.
        double progress() const noexcept;

        const CompletedTrace* latestResult() const noexcept
        {
            return m_latestResult ? &*m_latestResult : nullptr;
        }

        bool resultIsCurrent() const noexcept
        {
            return m_latestResult
                && m_latestResult->revision == m_revision;
        }

        // Increments whenever a new result is published.
        // Renderers can use this to decide when to rebuild GPU data.
        std::uint64_t resultVersion() const noexcept
        {
            return m_resultVersion;
        }

        const std::string& errorMessage() const noexcept
        {
            return m_errorMessage;
        }

    private:
        using Clock = std::chrono::steady_clock;

        struct JobOutcome
        {
            std::optional<CompletedTrace> completed;
            bool cancelled = false;
            std::string error;
        };

        void startJob(
            const telescope::TelescopeProject& project,
            const TraceSettings& settings);

        void pollJob();

        // Worker entry point. Does not access this controller or the UI.
        static JobOutcome runJob(
            TraceSnapshot snapshot,
            std::shared_ptr<TraceControl> control);

        TraceStatus m_status = TraceStatus::Idle;

        bool m_autoUpdate = true;
        bool m_resultsNeeded = false;

        bool m_dirty = true;
        bool m_traceRequested = false;

        std::uint64_t m_revision = 1;
        std::uint64_t m_resultVersion = 0;

        Clock::time_point m_lastChange = Clock::now();
        std::chrono::milliseconds m_debounceDelay{ 200 };

        std::size_t m_activeRayCount = 0;

        std::shared_ptr<TraceControl> m_control;
        std::future<JobOutcome> m_job;

        std::optional<CompletedTrace> m_latestResult;
        std::string m_errorMessage;
    };
}