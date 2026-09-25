// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "TelescopeProject.h"
namespace opticforge::telescope {
    void TelescopeProject::clearProject()
    {
        m_primitives.clear();
        m_nextPrimitive = 0; 

        // Launch pupil: 500 mm diameter, launching toward world +Z.
        m_launchPupil = LaunchPupil{};
        m_launchPupil.transform.setPosition(
            glm::dvec3(0.0, 250.0, -500.0));

        m_launchPupil.aperture = optics::Aperture{
            optics::CircularAperture{125.0} // Radius in mm.
        };

        m_launchPupil.localDirection =
            glm::dvec3(0.0, 0.0, 1.0);

        // Observation plane: local +Z normal faces world -Z.
        m_observationPlane = ObservationPlane{};
        m_observationPlane.transform.setPosition(
            glm::dvec3(0.0, 250.0, 500.0));

        m_observationPlane.transform.setEulerDegrees(
            glm::dvec3(0.0, 180.0, 0.0));

        m_observationPlane.displaySize =
            glm::dvec2(200.0, 200.0);

        // Capture the whole mathematical plane for spot diagrams.
        // The displayed square remains 200 x 200 mm.
        m_observationPlane.infiniteExtent = true;

        m_observationPlane.surface.setGeometry(
            optics::PlaneGeometry{});

        m_observationPlane.surface.opticalInterface().setDetector();

        // Incoming world +Z rays travel toward local -Z here.
        m_observationPlane.positiveCrossingOnly = false;
    }
}