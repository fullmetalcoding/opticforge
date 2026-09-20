#include "UI.h"
#include "imgui.h"
#include <cmath>
#include <type_traits>
#include <misc/cpp/imgui_stdlib.h>
#include <optics/SurfaceSeparation.h>

namespace opticforge::ui {
	namespace
	{
		static void updateFieldWidthFromPixelSize(renderer::PsfRenderSettings& s)
		{
			if (s.width <= 0)
				return;

			s.fieldWidth =
				s.pixelSizeMicrons *
				static_cast<double>(s.width) /
				1000.0;
		}

		static void updatePixelSizeFromFieldWidth(renderer::PsfRenderSettings& s)
		{
			if (s.width <= 0)
				return;

			s.pixelSizeMicrons =
				s.fieldWidth *
				1000.0 /
				static_cast<double>(s.width);
		}

		void drawOrientationInputs(
			const char* id,
			glm::dvec3& degrees)
		{
			ImGui::PushID(id);

			ImGui::Spacing();
			ImGui::TextUnformatted("Orientation");
			ImGui::Separator();

			ImGui::InputDouble(
				"Rotation X (deg)",
				&degrees.x,
				0.1,
				1.0,
				"%.6f");

			ImGui::InputDouble(
				"Rotation Y (deg)",
				&degrees.y,
				0.1,
				1.0,
				"%.6f");

			ImGui::InputDouble(
				"Rotation Z (deg)",
				&degrees.z,
				0.1,
				1.0,
				"%.6f");

			if (ImGui::Button("Reset orientation"))
			{
				degrees = glm::dvec3(0.0);
			}

			ImGui::PopID();
		}

		bool validOrientation(const glm::dvec3& degrees)
		{
			return
				std::isfinite(degrees.x) &&
				std::isfinite(degrees.y) &&
				std::isfinite(degrees.z);
		}
	}
	void UI::drawUI(telescope::TelescopeProject& project,
		bool& bQuit,
		raytracer::TraceController& traceController,
		raytracer::TraceSettings& traceSettings,
		const ProjectCommands& projectCommands,
		const SceneCommands& sceneCommands
	)
	{
		//Super janky. Refactor later to have an active menu dialog state. 
		bool openAddLensPopup = false;
		bool openAddMirrorPopup = false;
		bool openAboutPopup = false; 


		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New project...")){
					projectCommands.newProject();
					m_showPrimitiveManipulation = false;
					m_manipulatedPrimitiveId.reset();
				}
				if (ImGui::MenuItem("Open project...")) {
					projectCommands.openProject();
					m_showPrimitiveManipulation = false;
					m_manipulatedPrimitiveId.reset();
				}
				if (ImGui::MenuItem("Save project..."))
				{
					projectCommands.saveProject();
				}
				if (ImGui::MenuItem("Save project as..."))
				{
					projectCommands.saveProjectAs();
				}
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
			if (ImGui::BeginMenu("Trace"))
			{
				bool autoUpdate = traceController.autoUpdate();
				if (ImGui::MenuItem("Settings..."))
				{
					m_showTraceSettings = true;
				}

				if (ImGui::MenuItem("Auto Update", nullptr, &autoUpdate))
				{
					traceController.setAutoUpdate(autoUpdate);
				}

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("View")) {
				ImGui::MenuItem("Ray Paths", nullptr, &m_showRayPaths);
				ImGui::MenuItem("PSF trace...", nullptr, &m_showPsfTrace);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Help"))
			{
				if (ImGui::MenuItem("About OpticForge"))
				{
					openAboutPopup = true; 
				}

				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
		if (openAboutPopup) {
			ImGui::OpenPopup("About OpticForge");
		}
		if (ImGui::BeginPopupModal(
			"About OpticForge",
			nullptr,
			ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextUnformatted("OpticForge");
			ImGui::Separator();

			ImGui::Text("Version: %s", OPTICFORGE_VERSION_STRING);

#ifdef _DEBUG
			ImGui::TextUnformatted("Configuration: Debug");
#else
			ImGui::TextUnformatted("Configuration: Release");
#endif

			ImGui::Spacing();

			ImGui::TextUnformatted(
				"Open-source optical design and ray-tracing software."
			);

			ImGui::Spacing();

			if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		if (openAddLensPopup) {
			ImGui::OpenPopup("AddLens");
		}
		else if (openAddMirrorPopup) {
			ImGui::OpenPopup("AddMirror");
		}


		drawTraceSettingsWindow(project, traceController, traceSettings);
		drawAddLensPopup(project, traceController);
		drawAddMirrorPopup(project, traceController);
		drawPsfTraceWindow();
		drawPrimitiveManipulationWindow(
			project,
			traceController,
			sceneCommands);
	}
	void UI::drawAddLensPopup(
		telescope::TelescopeProject& project, raytracer::TraceController& control)
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
		
		ImGui::InputText("Name", &m_addLensDialog.name); 

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
		drawOrientationInputs(
			"LensOrientation",
			m_addLensDialog.orientationDegrees);

		ImGui::Spacing();
		ImGui::Separator();

		optics::SurfaceGeometry frontGeometry;

		if (m_addLensDialog.frontPlane)
		{
			frontGeometry = optics::PlaneGeometry{};
		}
		else
		{
			frontGeometry =
				optics::ConicGeometry{
					m_addLensDialog.frontRadiusMm,
					m_addLensDialog.frontConicConstant
			};
		}

		optics::SurfaceGeometry rearGeometry;

		if (m_addLensDialog.rearPlane)
		{
			rearGeometry = optics::PlaneGeometry{};
		}
		else
		{
			rearGeometry =
				optics::ConicGeometry{
					m_addLensDialog.rearRadiusMm,
					m_addLensDialog.rearConicConstant
			};
		}

		const double outerRadius =
			m_addLensDialog.diameterMm * 0.5;

		const double innerRadius =
			m_addLensDialog.centralHoleMm * 0.5;

		const auto requiredSeparation =
			optics::minimumAxialSeparation(
				frontGeometry,
				rearGeometry,
				innerRadius,
				outerRadius);

		double minimumThickness = 0.0;

		if (requiredSeparation)
		{
			const double clearance =
				std::max(
					1.0e-6,
					m_addLensDialog.diameterMm *
					1.0e-6);

			minimumThickness =
				*requiredSeparation +
				clearance;

			if (m_addLensDialog.thicknessMm <
				minimumThickness)
			{
				m_addLensDialog.thicknessMm =
					minimumThickness;
			}

			ImGui::TextDisabled(
				"Minimum thickness for this geometry: %.6f mm",
				minimumThickness);
		}

		const bool valid =
			std::isfinite(m_addLensDialog.diameterMm) &&
			std::isfinite(m_addLensDialog.thicknessMm) &&
			std::isfinite(m_addLensDialog.centralHoleMm) &&
			std::isfinite(m_addLensDialog.refractiveIndex) &&

			m_addLensDialog.diameterMm > 0.0 &&
			m_addLensDialog.thicknessMm > 0.0 &&

			m_addLensDialog.centralHoleMm >= 0.0 &&
			m_addLensDialog.centralHoleMm <
			m_addLensDialog.diameterMm &&

			m_addLensDialog.refractiveIndex > 0.0 &&

			(
				m_addLensDialog.frontPlane ||
				m_addLensDialog.frontRadiusMm != 0.0
				) &&

			(
				m_addLensDialog.rearPlane ||
				m_addLensDialog.rearRadiusMm != 0.0
				) &&

			requiredSeparation.has_value() &&

			validOrientation(
				m_addLensDialog.orientationDegrees);

		if (!requiredSeparation)
		{
			ImGui::TextColored(
				ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
				"Surface geometry is not defined over the full aperture!");
		}

		if (!valid)
			ImGui::BeginDisabled();

		if (ImGui::Button("Add"))
		{
			telescope::Lens lens;

			lens.transform.setPosition(
				m_addLensDialog.positionMm);

			lens.transform.setEulerDegrees(
				m_addLensDialog.orientationDegrees);

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
				std::move(lens),
				m_addLensDialog.name);
			control.invalidate();

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
		telescope::TelescopeProject& project, raytracer::TraceController& control)
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

		ImGui::InputText("Name", &m_addMirrorDialog.name);
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
		drawOrientationInputs(
			"MirrorOrientation",
			m_addMirrorDialog.orientationDegrees);

		ImGui::Spacing();
		ImGui::Separator();

		//
// -------------------------------------------------------------------------
// Geometry validation / minimum substrate thickness
// -------------------------------------------------------------------------
//

		const double outerRadius =
			m_addMirrorDialog.diameterMm * 0.5;

		const double innerRadius =
			m_addMirrorDialog.centralHoleMm * 0.5;

		//
		// Construct exactly the same surface geometry that will ultimately
		// be stored in the mirror.
		//
		// Keeping this here means the validation calculation and the actual
		// primitive construction cannot accidentally use different radius
		// sign conventions.
		//
		optics::SurfaceGeometry mirrorGeometry;

		if (m_addMirrorDialog.surfacePlane)
		{
			mirrorGeometry =
				optics::PlaneGeometry{};
		}
		else
		{
			double signedRadius =
				std::abs(
					m_addMirrorDialog.radiusMm);

			if (m_addMirrorDialog.curvature ==
				MirrorCurvature::Concave)
			{
				signedRadius =
					-signedRadius;
			}

			mirrorGeometry =
				optics::ConicGeometry{
					signedRadius,
					m_addMirrorDialog.conicConstant
			};
		}

		//
		// The mirror mesh uses:
		//
		//     optical surface: z = sag(r)
		//     rear surface:    z = thickness
		//
		// Therefore we require:
		//
		//     thickness > sag(r)
		//
		// everywhere across the usable aperture.
		//
		// minimumAxialSeparation() calculates the maximum required axial
		// separation across the complete radial interval.
		//
		const auto requiredSeparation =
			optics::minimumAxialSeparation(
				mirrorGeometry,
				optics::SurfaceGeometry{
					optics::PlaneGeometry{}
				},
				innerRadius,
				outerRadius);

		//
		// Add a small positive clearance.
		//
		// We do not want the optical surface to be merely tangent to the
		// rear substrate because the renderer ultimately converts mesh
		// coordinates to float.
		//
		double minimumThickness = 0.0;

		if (requiredSeparation.has_value())
		{
			const double clearance =
				std::max(
					1.0e-6,
					m_addMirrorDialog.diameterMm *
					1.0e-6);

			minimumThickness =
				*requiredSeparation +
				clearance;

			//
			// The user is allowed to increase thickness freely, but never
			// reduce it below the geometrically valid minimum.
			//
			if (m_addMirrorDialog.thicknessMm <
				minimumThickness)
			{
				m_addMirrorDialog.thicknessMm =
					minimumThickness;
			}

			//
			// Only show a geometrically meaningful minimum if the optical
			// sag actually imposes one.
			//
			if (*requiredSeparation > 0.0)
			{
				ImGui::TextDisabled(
					"Minimum thickness for this geometry: %.6f mm",
					minimumThickness);
			}
		}
		else
		{
			//
			// nullopt means one of the requested surface points does not
			// exist over the requested aperture -- for example a spherical
			// surface whose aperture exceeds the real domain of the sphere.
			//
			ImGui::TextColored(
				ImVec4(
					1.0f,
					0.4f,
					0.4f,
					1.0f),
				"Optical surface is not defined over the full aperture.");
		}

		//
		// -------------------------------------------------------------------------
		// General input validation
		// -------------------------------------------------------------------------
		//

		const bool valid =
			//
			// All basic numeric fields must be finite.
			//
			std::isfinite(
				m_addMirrorDialog.diameterMm) &&

			std::isfinite(
				m_addMirrorDialog.thicknessMm) &&

			std::isfinite(
				m_addMirrorDialog.centralHoleMm) &&

			std::isfinite(
				m_addMirrorDialog.radiusMm) &&

			std::isfinite(
				m_addMirrorDialog.conicConstant) &&

			//
			// Physical dimensions.
			//
			m_addMirrorDialog.diameterMm > 0.0 &&

			m_addMirrorDialog.thicknessMm > 0.0 &&

			m_addMirrorDialog.centralHoleMm >= 0.0 &&

			m_addMirrorDialog.centralHoleMm <
			m_addMirrorDialog.diameterMm &&

			//
			// Curved surfaces require a non-zero radius.
			//
			(
				m_addMirrorDialog.surfacePlane ||
				m_addMirrorDialog.radiusMm != 0.0
				) &&

			//
			// The conic must actually exist over the requested aperture.
			//
			requiredSeparation.has_value() &&

			//
			// Defensive check even though thickness was clamped above.
			//
			(
				!requiredSeparation.has_value() ||
				m_addMirrorDialog.thicknessMm >
				*requiredSeparation
				) &&

			validOrientation(
				m_addMirrorDialog.orientationDegrees);

		if (!valid)
		{
			ImGui::TextDisabled(
				"Diameter and thickness must be positive; "
				"the central hole must be smaller than the diameter; "
				"and curved surfaces must have valid geometry.");

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

			mirror.transform.setEulerDegrees(
				m_addMirrorDialog.orientationDegrees);

			mirror.thickness =
				m_addMirrorDialog.thicknessMm;

			mirror.centralHole =
				m_addMirrorDialog.centralHoleMm;

			//
			// Reuse the exact geometry that was validated above.
			//
			mirror.surface.setGeometry(
				mirrorGeometry);

			//
			// Finite optical aperture.
			//
			if (m_addMirrorDialog.centralHoleMm > 0.0)
			{
				mirror.surface.setAperture(
					optics::Aperture{
						optics::AnnularAperture{
							innerRadius,
							outerRadius
						}
					});
			}
			else
			{
				mirror.surface.setAperture(
					optics::Aperture{
						optics::CircularAperture{
							outerRadius
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
				std::move(mirror),
				m_addMirrorDialog.name);

			control.invalidate();

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
			bool changed = false;

			int background =
				static_cast<int>(m_psfSettings.background);

			if (ImGui::Combo(
				"Background",
				&background,
				"Black\0White\0"))
			{
				m_psfSettings.background =
					static_cast<renderer::PsfBackground>(background);

				changed = true;
			}

			int mark = static_cast<int>(m_psfSettings.mark);

			if (ImGui::Combo(
				"Render as",
				&mark,
				"Single pixel\0Gaussian splat\0"))
			{
				m_psfSettings.mark =
					static_cast<renderer::PsfMark>(mark);

				changed = true;
			}

			if (m_psfSettings.mark == renderer::PsfMark::Gaussian)
			{
				float sigma =
					static_cast<float>(m_psfSettings.sigmaPixels);

				if (ImGui::SliderFloat(
					"Sigma (pixels)",
					&sigma,
					0.25f,
					8.0f,
					"%.2f",
					ImGuiSliderFlags_AlwaysClamp))
				{
					m_psfSettings.sigmaPixels = sigma;
					changed = true;
				}
			}
			changed |= ImGui::Checkbox(
				"Auto center",
				&m_psfSettings.autoCenter);
			changed |= ImGui::Checkbox(
				"Auto fit",
				&m_psfSettings.autoFit);

			if (m_psfSettings.autoFit)
			{
				ImGui::SameLine();

				if (
					m_psfFieldSize.x > 0.0 &&
					m_psfTraceWidth > 0)
				{
					const double actualPixelSizeMicrons =
						m_psfFieldSize.x *
						1000.0 /
						static_cast<double>(m_psfTraceWidth);

					ImGui::TextDisabled(
						"Actual: %.4f mm  (%.3f um/px)",
						m_psfFieldSize.x,
						actualPixelSizeMicrons);
				}
				else
				{
					ImGui::TextDisabled("Actual: --");
				}
			}

			float field =
				static_cast<float>(m_psfSettings.fieldWidth);
			if (!m_psfSettings.autoFit) {
				if (ImGui::InputDouble("Sensor Pixel Size (um)", &m_psfSettings.pixelSizeMicrons,
					0.01, .1, "%.4f")) {
					m_psfSettings.pixelSizeMicrons =
						std::max(m_psfSettings.pixelSizeMicrons, 0.001);

					updateFieldWidthFromPixelSize(m_psfSettings);
				}
			}
			
			if (ImGui::SliderFloat(
				m_psfSettings.autoFit
				? "Minimum width (mm)"
				: "Field width (mm)",
				&field,
				0.001f,
				200.0f,
				"%.3f",
				ImGuiSliderFlags_Logarithmic |
				ImGuiSliderFlags_AlwaysClamp))
			{

				m_psfSettings.fieldWidth = field;
				updatePixelSizeFromFieldWidth(m_psfSettings); 
				changed = true;
			}



			if (!m_psfSettings.autoCenter)
			{
				double centerX = m_psfSettings.center.x;
				double centerY = m_psfSettings.center.y;

				if (ImGui::InputDouble(
					"Center X (mm)",
					&centerX) &&
					std::isfinite(centerX))
				{
					m_psfSettings.center.x = centerX;
					changed = true;
				}

				if (ImGui::InputDouble(
					"Center Y (mm)",
					&centerY) &&
					std::isfinite(centerY))
				{
					m_psfSettings.center.y = centerY;
					changed = true;
				}
			}
			changed |= ImGui::Checkbox(
				"Normalize peak",
				&m_psfSettings.normalizePeak);

			float exposure =
				static_cast<float>(m_psfSettings.exposure);

			if (ImGui::SliderFloat(
				"Display exposure",
				&exposure,
				0.01f,
				100.0f,
				"%.2f",
				ImGuiSliderFlags_Logarithmic |
				ImGuiSliderFlags_AlwaysClamp))
			{
				m_psfSettings.exposure = exposure;
				changed = true;
			}

			if (changed)
				++m_psfSettingsVersion;

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
	void UI::drawPrimitiveManipulationWindow(
		telescope::TelescopeProject& project,
		raytracer::TraceController& control,
		const SceneCommands& sceneCommands)
	{
		if (
			!m_showPrimitiveManipulation ||
			!m_manipulatedPrimitiveId)
		{
			return;
		}

		const telescope::PrimitiveId id =
			*m_manipulatedPrimitiveId;

		const std::string primitiveName = project.getNameForId(id); 

		telescope::TelescopePrimitive* primitive =
			project.findPrimitive(id);

		//
		// The project may have changed underneath the window.
		//
		if (primitive == nullptr)
		{
			m_showPrimitiveManipulation = false;
			m_manipulatedPrimitiveId.reset();
			return;
		}

		ImGui::SetNextWindowSize(
			ImVec2(360.0f, 0.0f),
			ImGuiCond_FirstUseEver);

		bool open =
			m_showPrimitiveManipulation;

		if (ImGui::Begin(
			"Primitive manipulation",
			&open,
			ImGuiWindowFlags_AlwaysAutoResize))
		{
			const char* typeName =
				std::visit(
					[](const auto& value) -> const char*
					{
						using T =
							std::decay_t<
							decltype(value)>;

						if constexpr (
							std::is_same_v<
							T,
							telescope::Lens>)
						{
							return "Lens";
						}
						else if constexpr (
							std::is_same_v<
							T,
							telescope::Mirror>)
						{
							return "Mirror";
						}
						else if constexpr (
							std::is_same_v<
							T,
							telescope::Detector>)
						{
							return "Detector";
						}
						else
						{
							return "Primitive";
						}
					},
					*primitive);

			ImGui::Text(
				"%s",
				typeName);

			ImGui::SameLine();

			ImGui::TextDisabled(
				"%s (ID %llu)",
				primitiveName.c_str(),
				static_cast<unsigned long long>(
					id));

			optics::Transform* transform =
				std::visit(
					[](auto& value)
					-> optics::Transform*
					{
						return &value.transform;
					},
					*primitive);

			//
			// ---------------------------------------------------------
			// Translation
			// ---------------------------------------------------------
			//

			ImGui::Spacing();

			ImGui::TextUnformatted(
				"Position");

			ImGui::Separator();

			glm::dvec3 position =
				transform->position();

			bool positionChanged =
				false;

			positionChanged |=
				ImGui::InputDouble(
					"X (mm)",
					&position.x,
					0.1,
					1.0,
					"%.6f");

			positionChanged |=
				ImGui::InputDouble(
					"Y (mm)",
					&position.y,
					0.1,
					1.0,
					"%.6f");

			positionChanged |=
				ImGui::InputDouble(
					"Z (mm)",
					&position.z,
					0.1,
					1.0,
					"%.6f");

			const bool finitePosition =
				std::isfinite(position.x) &&
				std::isfinite(position.y) &&
				std::isfinite(position.z);

			if (
				positionChanged &&
				finitePosition)
			{
				transform->setPosition(
					position);

				control.invalidate();
			}

			if (!finitePosition)
			{
				ImGui::TextDisabled(
					"Position values must be finite.");
			}

			//
			// ---------------------------------------------------------
			// Rotation
			// ---------------------------------------------------------
			//

			ImGui::Spacing();

			ImGui::TextUnformatted(
				"Orientation");

			ImGui::Separator();

			glm::dvec3 rotation =
				transform->eulerDegrees();

			bool rotationChanged =
				false;

			rotationChanged |=
				ImGui::InputDouble(
					"Rotation X (deg)",
					&rotation.x,
					0.01,
					0.1,
					"%.6f");

			rotationChanged |=
				ImGui::InputDouble(
					"Rotation Y (deg)",
					&rotation.y,
					0.01,
					0.1,
					"%.6f");

			rotationChanged |=
				ImGui::InputDouble(
					"Rotation Z (deg)",
					&rotation.z,
					0.01,
					0.1,
					"%.6f");

			const bool finiteRotation =
				std::isfinite(rotation.x) &&
				std::isfinite(rotation.y) &&
				std::isfinite(rotation.z);

			if (
				rotationChanged &&
				finiteRotation)
			{
				transform->setEulerDegrees(
					rotation);

				control.invalidate();
			}

			if (!finiteRotation)
			{
				ImGui::TextDisabled(
					"Rotation values must be finite.");
			}

			if (ImGui::Button(
				"Reset orientation"))
			{
				transform->setEulerDegrees(
					glm::dvec3(0.0));

				control.invalidate();
			}

			//
			// ---------------------------------------------------------
			// Delete
			// ---------------------------------------------------------
			//

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Button(
				"Delete primitive"))
			{
				ImGui::OpenPopup(
					"ConfirmDeletePrimitive");
			}

			if (ImGui::BeginPopupModal(
				"ConfirmDeletePrimitive",
				nullptr,
				ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text(
					"Delete %s \"%s\" (ID % llu) ? ",
					typeName,
					primitiveName.c_str(),
					static_cast<unsigned long long>(
						id));

				ImGui::TextUnformatted(
					"This cannot be undone.");

				ImGui::Spacing();

				if (ImGui::Button(
					"Delete"))
				{
					if (
						project.removePrimitive(
							id))
					{
						control.invalidate();

						if (
							sceneCommands.primitiveDeleted)
						{
							sceneCommands.primitiveDeleted(
								id);
						}
					}

					m_manipulatedPrimitiveId.reset();
					m_showPrimitiveManipulation = false;

					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();

				if (ImGui::Button(
					"Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}

		ImGui::End();

		if (!open)
		{
			m_showPrimitiveManipulation = false;
			m_manipulatedPrimitiveId.reset();
		}
	}
}