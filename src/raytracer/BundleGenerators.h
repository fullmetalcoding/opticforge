#pragma once
#include <vector>
#include "optics/Ray.h"

namespace opticforge::raytracer {
	using RayBundle =
		std::vector<optics::OpticalRay>;

    RayBundle generatePupilRayBundle(
        std::size_t numberOfRays,
        double pupilDiameter,
        double pupilDistanceMinusZ,
        double pupilElevationY,
        double offAxisAngleXRadians,
        double offAxisAngleYRadians);
}