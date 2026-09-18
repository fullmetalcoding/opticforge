#include "UI.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <thread>

namespace opticforge::ui
{
    void UI::drawTraceSettingsWindow(telescope::TelescopeProject& project,
        raytracer::TraceController& control, raytracer::TraceSettings& settings)
    {
        if (!m_showTraceSettings) return;
        ImGui::SetNextWindowSize(ImVec2(520, 690), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Raytracer settings", &m_showTraceSettings))
        {
            bool traceChanged = false;
            if (ImGui::CollapsingHeader("Ray bundle", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // Commit text fields with Enter; avoid starting jobs for partially
                // typed numbers. Arrow buttons still work normally.
                constexpr auto commit = ImGuiInputTextFlags_EnterReturnsTrue;
                int count = static_cast<int>(std::min<std::size_t>(settings.rayCount, 1000000));
                if (ImGui::InputInt("Number of rays", &count, 100, 1000))
                {
                    settings.rayCount = static_cast<std::size_t>(std::clamp(count, 1, 1000000));
                    traceChanged = true;
                }
                auto seed = settings.randomSeed;
                if (ImGui::InputScalar("Monte Carlo seed", ImGuiDataType_U32, &seed,
                    nullptr, nullptr, "%u"))
                {
                    settings.randomSeed = seed;
                    traceChanged = true;
                }
                int workers = static_cast<int>(settings.workerCount);
                const auto hardware = std::max(1u, std::thread::hardware_concurrency());
                if (ImGui::InputInt("Worker threads (0 = auto)", &workers, 1, 4))
                {
                    settings.workerCount = static_cast<unsigned>(
                        std::clamp(workers, 0, static_cast<int>(hardware)));
                    traceChanged = true;
                }
                ImGui::Text("Available logical CPUs: %u", hardware);
                ImGui::TextDisabled("Auto leaves one logical CPU free where possible.");
                int interactions = static_cast<int>(settings.maxInteractions);
                if (ImGui::InputInt("Maximum interactions", &interactions, 1, 10))
                {
                    settings.maxInteractions = static_cast<std::uint32_t>(
                        std::clamp(interactions, 1, 10000));
                    traceChanged = true;
                }
                double wavelength = settings.wavelengthNm;
                if (ImGui::InputDouble("Wavelength (nm)", &wavelength, 1, 10, "%.3f")
                    && std::isfinite(wavelength) && wavelength > 0.0)
                {
                    settings.wavelengthNm = wavelength;
                    traceChanged = true;
                }

                auto pupil = project.getLaunchPupil();
                const auto direction = pupil.localDirection;
                double angleX = glm::degrees(std::atan2(direction.x, direction.z));
                double angleY = glm::degrees(std::atan2(direction.y, direction.z));
                bool angleChanged = ImGui::InputDouble("Field X (deg)", &angleX,
                    0.01, 0.1, "%.6f");
                angleChanged |= ImGui::InputDouble("Field Y (deg)", &angleY,
                    0.01, 0.1, "%.6f");
                if (angleChanged && std::isfinite(angleX) && std::isfinite(angleY)
                    && std::abs(angleX) < 89.0 && std::abs(angleY) < 89.0)
                {
                    // Projected field angles relative to the pupil's local +Z.
                    pupil.localDirection = glm::normalize(glm::dvec3(
                        std::tan(glm::radians(angleX)),
                        std::tan(glm::radians(angleY)), 1.0));
                    project.setLaunchPupil(pupil);
                    traceChanged = true;
                }
                ImGui::TextDisabled("Field angles are relative to pupil +Z; range (-89, 89).");
                ImGui::TextDisabled("Press Enter to commit typed values.");
            }

            if (ImGui::CollapsingHeader("Launch pupil", ImGuiTreeNodeFlags_DefaultOpen))
            {
                constexpr auto commit = ImGuiInputTextFlags_EnterReturnsTrue;
                auto pupil = project.getLaunchPupil();
                auto position = pupil.transform.position();
                auto rotation = pupil.transform.eulerDegrees();
                bool moved = ImGui::InputDouble("Position X (mm)##pupil", &position.x, 1, 10, "%.6f");
                moved |= ImGui::InputDouble("Position Y (mm)##pupil", &position.y, 1, 10, "%.6f");
                moved |= ImGui::InputDouble("Position Z (mm)##pupil", &position.z, 1, 10, "%.6f");
                bool rotated = ImGui::InputDouble("Rotation X (deg)##pupil", &rotation.x, 0.1, 1, "%.6f");
                rotated |= ImGui::InputDouble("Rotation Y (deg)##pupil", &rotation.y, 0.1, 1, "%.6f");
                rotated |= ImGui::InputDouble("Rotation Z (deg)##pupil", &rotation.z, 0.1, 1, "%.6f");
                const auto finite = [](const glm::dvec3& v)
                    { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); };
                bool pupilChanged = false;
                if (moved && finite(position))
                {
                    pupil.transform.setPosition(position); pupilChanged = true;
                }
                if (rotated && finite(rotation))
                {
                    pupil.transform.setEulerDegrees(rotation); pupilChanged = true;
                }
                if (const auto* circle = std::get_if<optics::CircularAperture>(&pupil.aperture.geometry()))
                {
                    double diameter = 2.0 * circle->radius;
                    if (ImGui::InputDouble("Diameter (mm)", &diameter, 1, 10, "%.6f")
                        && std::isfinite(diameter) && diameter > 0.0)
                    {
                        pupil.aperture = optics::Aperture{ optics::CircularAperture{diameter * 0.5} };
                        pupilChanged = true;
                    }
                }
                else ImGui::TextDisabled("Diameter editing requires a circular launch pupil.");
                if (pupilChanged)
                {
                    project.setLaunchPupil(pupil);
                    traceChanged = true;
                }
            }
            if (ImGui::CollapsingHeader("Observation Plane", ImGuiTreeNodeFlags_DefaultOpen))
            {
                
                auto obsPlane = project.getObservationPlane();
                auto position = obsPlane.transform.position();
                auto rotation = obsPlane.transform.eulerDegrees();
                bool moved = ImGui::InputDouble("Position X (mm)##obsplane", &position.x, 1, 10, "%.6f");
                moved |= ImGui::InputDouble("Position Y (mm)##obsplane", &position.y, 1, 10, "%.6f");
                moved |= ImGui::InputDouble("Position Z (mm)##obsplane", &position.z, 1, 10, "%.6f");
                bool rotated = ImGui::InputDouble("Rotation X (deg)##obsplane", &rotation.x, 0.1, 1, "%.6f");
                rotated |= ImGui::InputDouble("Rotation Y (deg)##obsplane", &rotation.y, 0.1, 1, "%.6f");
                rotated |= ImGui::InputDouble("Rotation Z (deg)##obsplane", &rotation.z, 0.1, 1, "%.6f");
                const auto finite = [](const glm::dvec3& v)
                    { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); };
                bool obsPlaneChanged = false;
                if (moved && finite(position))
                {
                    obsPlane.transform.setPosition(position); obsPlaneChanged = true;
                }
                if (rotated && finite(rotation))
                {
                    obsPlane.transform.setEulerDegrees(rotation); obsPlaneChanged = true;
                }
                /*
                if (const auto* circle = std::get_if<optics::CircularAperture>(&obsPlane.aperture.geometry()))
                {
                    double diameter = 2.0 * circle->radius;
                    if (ImGui::InputDouble("Diameter (mm)", &diameter, 1, 10, "%.6f")
                        && std::isfinite(diameter) && diameter > 0.0)
                    {
                        pupil.aperture = optics::Aperture{ optics::CircularAperture{diameter * 0.5} };
                        pupilChanged = true;
                    }
                }
                else ImGui::TextDisabled("Diameter editing requires a circular launch pupil.");
                */
                if (obsPlaneChanged)
                {
                    project.setObservationPlane(obsPlane);
                    traceChanged = true;
                }
            }

            if (ImGui::CollapsingHeader("Ray paths", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Show ray paths", &m_showRayPaths);
                int maxRays = m_rayPathSettings.maxRays;
                if (ImGui::InputInt("Maximum displayed rays", &maxRays, 100, 1000))
                {
                    m_rayPathSettings.maxRays = std::clamp(maxRays, 0, 100000);
                    ++m_rayPathGeometryVersion;
                }
                ImGui::ColorEdit4("Observation plane", &m_rayPathSettings.observationColor.x);
                ImGui::ColorEdit4("Escaped rays", &m_rayPathSettings.escapedColor.x);
                ImGui::ColorEdit4("Other terminations", &m_rayPathSettings.terminatedColor.x);
                ImGui::TextDisabled("Color alpha controls additive brightness per ray.");
                ImGui::SliderFloat("Line thickness (px)", &m_rayPathSettings.lineWidth,
                    1.0f, 10.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
                ImGui::TextDisabled("Line thickness is limited by your OpenGL driver.");
                double length = m_rayPathSettings.escapeLengthMm;
                if (ImGui::InputDouble("Escape extension (mm)", &length, 10, 100, "%.3f") && std::isfinite(length) && length >= 0)
                {
                    m_rayPathSettings.escapeLengthMm = length;
                    ++m_rayPathGeometryVersion;
                }
                ImGui::Checkbox("Depth test against scene", &m_rayPathSettings.depthTest);
                ImGui::Text("Displayed rays: %zu", m_displayedRays);
                if (m_rayPathsTruncated)
                    ImGui::TextWrapped("Display limited to one million line segments. Reduce displayed rays.");
            }
            ImGui::Separator();
            ImGui::TextDisabled("PSF spot controls remain in the PSF viewer.");
            if (traceChanged) control.invalidate();
            if (ImGui::Button("Trace now")) control.requestTrace();
            ImGui::SameLine();
            bool automatic = control.autoUpdate();
            if (ImGui::Checkbox("Auto update", &automatic)) control.setAutoUpdate(automatic);
            if (!automatic)
                ImGui::TextDisabled("Changes mark results stale. Use Trace now to recompute.");
            else if (!m_showPsfTrace && !m_showRayPaths)
                ImGui::TextDisabled("Open a result view or use Trace now to start tracing.");
            if (control.isRunning())
                ImGui::ProgressBar(static_cast<float>(control.progress()));
            if (control.latestResult() && !control.resultIsCurrent())
                ImGui::TextDisabled("Showing the previous solution while results are stale.");
            if (!control.errorMessage().empty())
                ImGui::TextWrapped("Trace failed: %s", control.errorMessage().c_str());
        }
        ImGui::End();
    }
}
