#pragma once
#include "telescope/TelescopeProject.h"
#include "raytracer/TraceController.h"

namespace opticforge::ui
{
  
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
    };

	class UI {
	public:
        void drawUI(telescope::TelescopeProject& project, bool & bQuit, raytracer::TraceController & control);
        // The renderer owns this texture and must keep it alive while displayed.
        void setPsfTraceTexture(unsigned int texture, int width, int height)
        {
            m_psfTraceTexture = texture;
            m_psfTraceWidth = width;
            m_psfTraceHeight = height;
        }
        bool showPsf() {
            return m_showPsfTrace;
        }
        bool showRays() {
            return m_showRayPaths;
        }

	private:
        void drawAddLensPopup(
            telescope::TelescopeProject& project, raytracer::TraceController& control);
        void drawAddMirrorPopup(
            telescope::TelescopeProject& project, raytracer::TraceController& control);

		AddLensDialogState m_addLensDialog;
        AddMirrorDialogState m_addMirrorDialog;

        void drawPsfTraceWindow();

        bool m_showPsfTrace = false;
        bool m_showRayPaths = false; 


        unsigned int m_psfTraceTexture = 0;
        int m_psfTraceWidth = 512;
        int m_psfTraceHeight = 512;
	};
}