#pragma once
#include "telescope/TelescopeProject.h"

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
        void drawUI(telescope::TelescopeProject& project, bool & bQuit);
	private:
        void drawAddLensPopup(
            telescope::TelescopeProject& project);
        void drawAddMirrorPopup(
            telescope::TelescopeProject& project); 

		AddLensDialogState m_addLensDialog;
        AddMirrorDialogState m_addMirrorDialog;
	};
}