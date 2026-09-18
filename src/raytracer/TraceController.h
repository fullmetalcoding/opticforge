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

        // 0 = automatic.
        //
        // Automatic mode leaves one logical CPU available for the
        // UI/render thread where possible.
        unsigned int workerCount = 0;
    };


    // Shared between the controller and the background trace job.
    //
    // Contains no telescope/project/rendering state, so the background
    // workers never need to access the TraceController itself.
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


    // Stable copy of everything required by one trace.
    //
    // The UI/project can continue changing while workers operate on this
    // snapshot.
    struct TraceSnapshot
    {
        std::vector<telescope::PrimitiveRecord> primitives;

        telescope::LaunchPupil launchPupil;
        telescope::ObservationPlane observationPlane;

        TraceSettings settings;

        // Identifies the project/settings configuration used for this run.
        std::uint64_t revision = 0;
    };


    struct CompletedTrace
    {
        TraceResult result;

        // Preserve the exact observation-plane coordinate frame associated
        // with the result.
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


        // Mark the current result stale because project geometry or trace
        // settings changed.
        //
        // If a trace is currently running it is asked to cancel. Automatic
        // replacement is started by update() after the debounce interval.
        void invalidate();


        // Request an immediate trace.
        //
        // This bypasses the automatic-update debounce. If another job is
        // running, it is cancelled first and this request remains pending.
        void requestTrace();


        // Cancel the active job and suppress any pending automatic restart
        // until another edit/request occurs.
        void requestCancel();


        void setAutoUpdate(bool enabled);


        // True whenever some view requires a current ray-trace result.
        //
        // At present this normally corresponds to either the PSF viewer or
        // the ray-path renderer being visible.
        void setResultsNeeded(bool needed);


        // Called once per frame from the main/UI thread.
        //
        // This function never waits for the raytracer:
        //
        // 1. Poll the current future.
        // 2. Publish it if complete.
        // 3. Start a pending trace when eligible.
        void update(
            const telescope::TelescopeProject& project,
            const TraceSettings& settings);


        TraceStatus status() const noexcept
        {
            return m_status;
        }


        bool isRunning() const noexcept
        {
            return
                m_status == TraceStatus::Running ||
                m_status == TraceStatus::Cancelling;
        }


        bool autoUpdate() const noexcept
        {
            return m_autoUpdate;
        }


        // Fraction of rays completed by the currently active trace.
        double progress() const noexcept;


        const CompletedTrace* latestResult() const noexcept
        {
            return m_latestResult
                ? &*m_latestResult
                : nullptr;
        }


        bool resultIsCurrent() const noexcept
        {
            return
                m_latestResult &&
                m_latestResult->revision == m_revision;
        }


        // Incremented whenever a newly completed trace is published.
        //
        // Renderers can use this to avoid rebuilding geometry/textures unless
        // the ray solution actually changed.
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


        // Background-job entry point.
        //
        // runJob owns its TraceSnapshot and never accesses the controller,
        // TelescopeProject, UI, renderer, or OpenGL state.
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

        // Keep the existing debounce behavior for now.
        std::chrono::milliseconds m_debounceDelay{ 200 };

        std::size_t m_activeRayCount = 0;

        std::shared_ptr<TraceControl> m_control;

        std::future<JobOutcome> m_job;

        std::optional<CompletedTrace> m_latestResult;

        std::string m_errorMessage;
    };
}