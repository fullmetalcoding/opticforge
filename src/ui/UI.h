#pragma once
#include "telescope/TelescopeProject.h"
#include "raytracer/TraceController.h"
#include "renderer/PsfRasterizer.h"
#include "renderer/RayPathGeometry.h"
#include <functional>
#include <optional>


namespace opticforge::ui
{
    struct ProjectCommands
    {
        std::function<void()> newProject;
        std::function<void()> openProject;
        std::function<void()> saveProject;
        std::function<void()> saveProjectAs;
    };
    struct SceneCommands
    {
        std::function<void(telescope::PrimitiveId)>
            primitiveDeleted;
    };
  
    struct AddLensDialogState
    {
        double diameterMm = 200.0;
        double thicknessMm = 20.0;
        double centralHoleMm = 0.0;

        bool frontPlane = false;
        double frontRadiusMm = 826.9;
        double frontConicConstant = 0.0;

        bool rearPlane = true;
        double rearRadiusMm = -826.9;
        double rearConicConstant = 0.0;

        double refractiveIndex = 1.5168;

        glm::dvec3 positionMm{ 0.0, 0.0, 0.0 };
        glm::dvec3 orientationDegrees{ 0.0, 0.0, 0.0 };
        std::string name{ "Lens" }; 
    };
    enum class MirrorCurvature
    {
        Concave,
        Convex
    };
    struct AddMirrorDialogState
    {
        double diameterMm = 200.0;
        double thicknessMm = 20.0;
        double centralHoleMm = 0.0;

        bool surfacePlane = false;

        double radiusMm = 1600.0;
        double conicConstant = -1.0;

        MirrorCurvature curvature =
            MirrorCurvature::Concave; 

        glm::dvec3 positionMm{ 0.0, 0.0, 0.0 };
        glm::dvec3 orientationDegrees{ 0.0, 0.0, 0.0 };
        std::string name{ "Mirror" }; 
    };

	class UI {
	public:
        void drawUI(telescope::TelescopeProject& project, bool & bQuit, raytracer::TraceController & control,
            raytracer::TraceSettings & traceSettings, const ProjectCommands& projectCommands, const SceneCommands& sceneCommands);
        const renderer::RayPathRenderSettings& rayPathSettings() const
        {
            return m_rayPathSettings;
        }
        std::uint64_t rayPathGeometryVersion() const { return m_rayPathGeometryVersion; }
        void setRayPathStats(std::size_t rays, bool truncated)
        {
            m_displayedRays = rays; m_rayPathsTruncated = truncated;
        }
        // The renderer owns this texture and must keep it alive while displayed.
        void setPsfTraceTexture(unsigned int texture, int width, int height, glm::vec2 fieldsize)
        {
            m_psfTraceTexture = texture;
            m_psfTraceWidth = width;
            m_psfTraceHeight = height;
            m_psfFieldSize = fieldsize; 
        }
        bool showPsf() const {
            return m_showPsfTrace;
        }
        bool showRays() const {
            return m_showRayPaths;
        }
        const renderer::PsfRenderSettings& psfSettings() const
        {
            return m_psfSettings;
        }

        std::uint64_t psfSettingsVersion() const
        {
            return m_psfSettingsVersion;
        }

        void openPrimitiveManipulation(
            telescope::PrimitiveId id)
        {
            m_manipulatedPrimitiveId = id;
            m_showPrimitiveManipulation = true;
        }

	private:
        void drawTraceSettingsWindow(telescope::TelescopeProject& project,
            raytracer::TraceController& control, raytracer::TraceSettings& settings);
        bool m_showTraceSettings = false;
        renderer::RayPathRenderSettings m_rayPathSettings;
        std::uint64_t m_rayPathGeometryVersion = 1;
        std::size_t m_displayedRays = 0;

        bool m_rayPathsTruncated = false;
        void drawAddLensPopup(
            telescope::TelescopeProject& project, raytracer::TraceController& control);
        void drawAddMirrorPopup(
            telescope::TelescopeProject& project, raytracer::TraceController& control);

		AddLensDialogState m_addLensDialog;
        AddMirrorDialogState m_addMirrorDialog;

        void drawPsfTraceWindow();

        bool m_showPsfTrace = false;
        bool m_showRayPaths = false; 

        glm::vec2 m_psfFieldSize{ 0,0 };
        unsigned int m_psfTraceTexture = 0;
        int m_psfTraceWidth = 512;
        int m_psfTraceHeight = 512;
        renderer::PsfRenderSettings m_psfSettings;
        std::uint64_t m_psfSettingsVersion = 1;

        void drawPrimitiveManipulationWindow(
            telescope::TelescopeProject& project,
            raytracer::TraceController& control,
            const SceneCommands& sceneCommands);

        bool m_showPrimitiveManipulation = false;

        std::optional<telescope::PrimitiveId>
            m_manipulatedPrimitiveId;
	};
}