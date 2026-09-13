#include "UI.h"
#include "imgui.h"

namespace opticforge::ui {
    void UI::drawUI(telescope::TelescopeProject& project, bool & bQuit)
    {
        //Super janky. Refactor later to have an active menu dialog state. 
        bool openAddLensPopup = false;
        bool openAddMirrorPopup = false; 

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New project...")) {
                    project.clearProject(); 
                }
                if (ImGui::MenuItem("Open project..."));
                if (ImGui::MenuItem("Save project..."));
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) bQuit = true;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::BeginMenu("Add Primitive")) {
                    if (ImGui::MenuItem("Lens..."))
                    {
                        openAddLensPopup = true; 
                  
                    }
                    if (ImGui::MenuItem("Mirror...")) {
                        openAddMirrorPopup = true; 
                    }

                    ImGui::EndMenu();
                }
                ImGui::EndMenu();

            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("PSF trace", nullptr, &m_showPsfTrace);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (openAddLensPopup) {
            ImGui::OpenPopup("AddLens");
        }
        else if (openAddMirrorPopup) {
            ImGui::OpenPopup("AddMirror");
        }
        drawAddLensPopup(project); 
        drawAddMirrorPopup(project); 
        drawPsfTraceWindow();
    }
    void UI::drawAddLensPopup(
        telescope::TelescopeProject& project)
    {
        if (!ImGui::BeginPopupModal(
            "AddLens",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            return;
        }

        ImGui::TextUnformatted("Lens Geometry");
        ImGui::Separator();

        ImGui::InputDouble(
            "Diameter (mm)",
            &m_addLensDialog.diameterMm,
            1.0,
            10.0,
            "%.3f");

        ImGui::InputDouble(
            "Center Thickness (mm)",
            &m_addLensDialog.thicknessMm,
            0.1,
            1.0,
            "%.3f");

        ImGui::InputDouble(
            "Central Hole Diameter (mm)",
            &m_addLensDialog.centralHoleMm,
            1.0,
            10.0,
            "%.3f");

        ImGui::Spacing();

        ImGui::TextUnformatted("Front Surface");
        ImGui::Separator();

        ImGui::Checkbox(
            "Front surface is plane",
            &m_addLensDialog.frontPlane);

        if (!m_addLensDialog.frontPlane)
        {
            ImGui::InputDouble(
                "Front Radius (mm)",
                &m_addLensDialog.frontRadiusMm,
                1.0,
                10.0,
                "%.6f");

            ImGui::InputDouble(
                "Front Conic Constant",
                &m_addLensDialog.frontConicConstant,
                0.01,
                0.1,
                "%.6f");
        }

        ImGui::Spacing();

        ImGui::TextUnformatted("Rear Surface");
        ImGui::Separator();

        ImGui::Checkbox(
            "Rear surface is plane",
            &m_addLensDialog.rearPlane);

        if (!m_addLensDialog.rearPlane)
        {
            ImGui::InputDouble(
                "Rear Radius (mm)",
                &m_addLensDialog.rearRadiusMm,
                1.0,
                10.0,
                "%.6f");

            ImGui::InputDouble(
                "Rear Conic Constant",
                &m_addLensDialog.rearConicConstant,
                0.01,
                0.1,
                "%.6f");
        }

        ImGui::Spacing();

        ImGui::TextUnformatted("Optical Properties");
        ImGui::Separator();

        ImGui::InputDouble(
            "Refractive Index",
            &m_addLensDialog.refractiveIndex,
            0.001,
            0.01,
            "%.6f");

        ImGui::Spacing();

        ImGui::TextUnformatted("Position");
        ImGui::Separator();

        ImGui::InputDouble(
            "X (mm)",
            &m_addLensDialog.positionMm.x,
            1.0,
            10.0,
            "%.3f");

        ImGui::InputDouble(
            "Y (mm)",
            &m_addLensDialog.positionMm.y,
            1.0,
            10.0,
            "%.3f");

        ImGui::InputDouble(
            "Z (mm)",
            &m_addLensDialog.positionMm.z,
            1.0,
            10.0,
            "%.3f");

        ImGui::Spacing();
        ImGui::Separator();

        const bool valid =
            m_addLensDialog.diameterMm > 0.0 &&
            m_addLensDialog.thicknessMm > 0.0 &&
            m_addLensDialog.refractiveIndex > 0.0 &&
            m_addLensDialog.centralHoleMm >= 0.0; 

        if (!valid)
            ImGui::BeginDisabled();

        if (ImGui::Button("Add"))
        {
            telescope::Lens lens;

            lens.transform.setPosition(
                m_addLensDialog.positionMm);

            lens.centerThickness =
                m_addLensDialog.thicknessMm;

            const double outerRadius =
                m_addLensDialog.diameterMm * 0.5;

            const double innerRadius =
                m_addLensDialog.centralHoleMm * 0.5;

            //
            // Front surface
            //
            if (m_addLensDialog.frontPlane)
            {
                lens.frontSurface.setGeometry(
                    optics::PlaneGeometry{});
            }
            else
            {
                lens.frontSurface.setGeometry(
                    optics::ConicGeometry{
                        m_addLensDialog.frontRadiusMm,
                        m_addLensDialog.frontConicConstant
                    });
            }

            if (m_addLensDialog.centralHoleMm > 0.0)
            {
                const optics::Aperture aperture{
                    optics::AnnularAperture{
                        innerRadius,
                        outerRadius
                    }
                };

                lens.frontSurface.setAperture(aperture);
                lens.rearSurface.setAperture(aperture);
            }
            else {
                lens.frontSurface.setAperture(
                    optics::Aperture{
                        optics::CircularAperture{
                            outerRadius
                        }
                    });
            }

            lens.frontSurface.setOpticalInterface(
                optics::OpticalInterface{
                    optics::RefractiveInterface{
                        1.0,
                        m_addLensDialog.refractiveIndex
                    }
                });

            //
            // Rear surface
            //
            if (m_addLensDialog.rearPlane)
            {
                lens.rearSurface.setGeometry(
                    optics::PlaneGeometry{});
            }
            else
            {
                lens.rearSurface.setGeometry(
                    optics::ConicGeometry{
                        m_addLensDialog.rearRadiusMm,
                        m_addLensDialog.rearConicConstant
                    });
            }

            lens.rearSurface.setAperture(
                optics::Aperture{
                    optics::CircularAperture{
                        outerRadius
                    }
                });

            lens.rearSurface.setOpticalInterface(
                optics::OpticalInterface{
                    optics::RefractiveInterface{
                        m_addLensDialog.refractiveIndex,
                        1.0
                    }
                });

            project.addPrimitive(
                std::move(lens));

            ImGui::CloseCurrentPopup();
        }

        if (!valid)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    void UI::drawAddMirrorPopup(
        telescope::TelescopeProject& project)
    {
        if (!ImGui::BeginPopupModal(
            "AddMirror",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            return;
        }

        //
        // -------------------------------------------------------------------------
        // Physical geometry
        // -------------------------------------------------------------------------
        //

        ImGui::TextUnformatted("Mirror Geometry");
        ImGui::Separator();

        ImGui::InputDouble(
            "Diameter (mm)",
            &m_addMirrorDialog.diameterMm,
            1.0,
            10.0,
            "%.3f");

        ImGui::InputDouble(
            "Thickness (mm)",
            &m_addMirrorDialog.thicknessMm,
            1.0,
            10.0,
            "%.3f");
        ImGui::InputDouble(
            "Central Hole Diameter (mm)",
            &m_addMirrorDialog.centralHoleMm,
            1.0,
            10.0,
            "%.3f");

        ImGui::Spacing();

        //
        // -------------------------------------------------------------------------
        // Optical surface
        // -------------------------------------------------------------------------
        //

        ImGui::TextUnformatted("Optical Surface");
        ImGui::Separator();

        ImGui::Checkbox(
            "Plane surface",
            &m_addMirrorDialog.surfacePlane);

        if (!m_addMirrorDialog.surfacePlane)
        {
            ImGui::InputDouble(
                "Radius of Curvature (mm)",
                &m_addMirrorDialog.radiusMm,
                1.0,
                10.0,
                "%.6f");

            int curvatureChoice =
                m_addMirrorDialog.curvature ==
                MirrorCurvature::Concave
                ? 0
                : 1;
            ImGui::SameLine();

            if (ImGui::RadioButton(
                "Concave",
                curvatureChoice == 0))
            {
                m_addMirrorDialog.curvature =
                    MirrorCurvature::Concave;
            }

            ImGui::SameLine();

            if (ImGui::RadioButton(
                "Convex",
                curvatureChoice == 1))
            {
                m_addMirrorDialog.curvature =
                    MirrorCurvature::Convex;
            }


            ImGui::InputDouble(
                "Conic Constant",
                &m_addMirrorDialog.conicConstant,
                0.01,
                0.1,
                "%.6f");

            //
            // Some useful contextual information.
            //
            if (m_addMirrorDialog.radiusMm != 0.0)
            {
                const double focalLength =
                    std::abs(
                        m_addMirrorDialog.radiusMm) *
                    0.5;

                const double focalRatio =
                    focalLength /
                    m_addMirrorDialog.diameterMm;

                ImGui::Text(
                    "Approx. focal length: %.3f mm",
                    focalLength);

                if (m_addMirrorDialog.diameterMm > 0.0)
                {
                    ImGui::Text(
                        "Approx. focal ratio: f/%.3f",
                        focalRatio);
                }
            }

            ImGui::Spacing();

            ImGui::TextDisabled(
                "k = 0: sphere");

            ImGui::TextDisabled(
                "k = -1: paraboloid");

            ImGui::TextDisabled(
                "k < -1: hyperboloid");

            ImGui::TextDisabled(
                "-1 < k < 0: ellipsoid");
        }

        ImGui::Spacing();

        //
        // -------------------------------------------------------------------------
        // Position
        // -------------------------------------------------------------------------
        //

        ImGui::TextUnformatted("Position");
        ImGui::Separator();

        ImGui::InputDouble(
            "X (mm)",
            &m_addMirrorDialog.positionMm.x,
            1.0,
            10.0,
            "%.3f");

        ImGui::InputDouble(
            "Y (mm)",
            &m_addMirrorDialog.positionMm.y,
            1.0,
            10.0,
            "%.3f");

        ImGui::InputDouble(
            "Z (mm)",
            &m_addMirrorDialog.positionMm.z,
            1.0,
            10.0,
            "%.3f");

        ImGui::Spacing();
        ImGui::Separator();

        //
        // -------------------------------------------------------------------------
        // Validation
        // -------------------------------------------------------------------------
        //

        const bool valid =
            m_addMirrorDialog.diameterMm > 0.0 &&
            m_addMirrorDialog.thicknessMm > 0.0 &&
            m_addMirrorDialog.centralHoleMm >= 0.0 && 
            (
                m_addMirrorDialog.surfacePlane ||
                m_addMirrorDialog.radiusMm != 0.0 
               
                );

        if (!valid)
        {
            ImGui::TextDisabled(
                "Diameter, central hole size, and thickness must be positive, and "
                "a curved surface must have a non-zero radius.");

            ImGui::BeginDisabled();
        }

        //
        // -------------------------------------------------------------------------
        // Add
        // -------------------------------------------------------------------------
        //

        if (ImGui::Button("Add"))
        {
            telescope::Mirror mirror;

            mirror.transform.setPosition(
                m_addMirrorDialog.positionMm);

            mirror.thickness =
                m_addMirrorDialog.thicknessMm;

            const double apertureRadius =
                m_addMirrorDialog.diameterMm *
                0.5;

            const double innerRadius =
                m_addMirrorDialog.centralHoleMm * 0.5;

            //
            // Surface geometry.
            //
            if (m_addMirrorDialog.surfacePlane)
            {
                mirror.surface.setGeometry(
                    optics::PlaneGeometry{});
            }
            else
            {
                double signedRadius =
                    std::abs(m_addMirrorDialog.radiusMm);

                if (m_addMirrorDialog.curvature ==
                    MirrorCurvature::Concave)
                {
                    signedRadius = -signedRadius;
                }


                mirror.surface.setGeometry(
                    optics::ConicGeometry{
                        signedRadius,
                        m_addMirrorDialog.conicConstant
                    });
            }

            //
            // Finite mirror diameter.
            //
            if (m_addMirrorDialog.centralHoleMm > 0.0)
            {
                mirror.surface.setAperture(
                    optics::Aperture{
                        optics::AnnularAperture{
                            innerRadius,
                            apertureRadius
                        }
                    });
            }else
            {
                mirror.surface.setAperture(
                    optics::Aperture{
                        optics::CircularAperture{
                            apertureRadius
                        }
                    });
            }
       

            //
            // Reflective optical boundary.
            //
            mirror.surface.setOpticalInterface(
                optics::OpticalInterface{
                    optics::ReflectiveInterface{
                        1.0
                    }
                });

            project.addPrimitive(
                std::move(mirror));

            ImGui::CloseCurrentPopup();
        }

        if (!valid)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    void UI::drawPsfTraceWindow()
    {
        if (!m_showPsfTrace)
            return;

        ImGui::SetNextWindowSize(
            ImVec2(560.0f, 600.0f),
            ImGuiCond_FirstUseEver);

        if (ImGui::Begin("PSF trace", &m_showPsfTrace))
        {
            const bool hasImage =
                m_psfTraceTexture != 0 &&
                m_psfTraceWidth > 0 &&
                m_psfTraceHeight > 0;

            if (hasImage)
            {
                ImGui::Text(
                    "PSF image: %d x %d",
                    m_psfTraceWidth,
                    m_psfTraceHeight);
            }
            else
            {
                ImGui::TextUnformatted("No PSF trace results yet.");
            }

            ImGui::Separator();

            const ImVec2 available = ImGui::GetContentRegionAvail();

            if (available.x > 0.0f && available.y > 0.0f)
            {
                const float aspect = hasImage
                    ? static_cast<float>(m_psfTraceWidth) /
                    static_cast<float>(m_psfTraceHeight)
                    : 1.0f;

                ImVec2 size(available.x, available.x / aspect);

                if (size.y > available.y)
                {
                    size.y = available.y;
                    size.x = size.y * aspect;
                }

                const ImVec2 cursor = ImGui::GetCursorPos();

                ImGui::SetCursorPos(ImVec2(
                    cursor.x + (available.x - size.x) * 0.5f,
                    cursor.y + (available.y - size.y) * 0.5f));

                if (hasImage)
                {
                    // UVs for an OpenGL framebuffer texture:
                    // flip vertically for ImGui's top-left image origin.
                    ImGui::Image(
                        static_cast<ImTextureID>(m_psfTraceTexture),
                        size,
                        ImVec2(0.0f, 1.0f),
                        ImVec2(1.0f, 0.0f));
                }
                else
                {
                    const ImVec2 start = ImGui::GetCursorScreenPos();

                    ImGui::GetWindowDrawList()->AddRectFilled(
                        start,
                        ImVec2(start.x + size.x, start.y + size.y),
                        IM_COL32(0, 0, 0, 255));

                    ImGui::Dummy(size);
                }
            }
        }

        // Required even when Begin() returns false.
        ImGui::End();
    }
}