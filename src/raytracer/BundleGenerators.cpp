#include "BundleGenerators.h"
#include <cmath>
#include <cstddef>
#include <numbers>
#include <random>
#include <stdexcept>


#include <glm/glm.hpp>

namespace opticforge::raytracer {
    RayBundle generatePupilRayBundle(
        std::size_t numberOfRays,
        double pupilDiameter,
        double pupilDistanceMinusZ,
        double pupilElevationY,
        double offAxisAngleXRadians,
        double offAxisAngleYRadians)
    {
        if (numberOfRays == 0)
        {
            return {};
        }

        if (pupilDiameter <= 0.0)
        {
            throw std::invalid_argument(
                "pupilDiameter must be greater than zero.");
        }

        if (pupilDistanceMinusZ < 0.0)
        {
            throw std::invalid_argument(
                "pupilDistanceMinusZ must be non-negative.");
        }

        RayBundle bundle;
        bundle.reserve(numberOfRays);

        const double pupilRadius =
            pupilDiameter * 0.5;

        const glm::dvec3 pupilCenter(
            0.0,
            pupilElevationY,
            -pupilDistanceMinusZ);

        //
        // Field direction.
        //
        // thetaX controls angular offset in the X-Z plane.
        // thetaY controls angular offset in the Y-Z plane.
        //
        const glm::dvec3 rayDirection =
            glm::normalize(
                glm::dvec3(
                    std::tan(offAxisAngleXRadians),
                    std::tan(offAxisAngleYRadians),
                    1.0));

        std::random_device rd;
        std::mt19937_64 rng(rd());

        std::uniform_real_distribution<double>
            unitDistribution(0.0, 1.0);

        for (std::size_t i = 0;
            i < numberOfRays;
            ++i)
        {
            const double u =
                unitDistribution(rng);

            const double v =
                unitDistribution(rng);

            //
            // Uniform-area sampling of the circular pupil.
            //
            const double radius =
                pupilRadius *
                std::sqrt(u);

            const double theta =
                2.0 *
                std::numbers::pi *
                v;

            const double x =
                radius *
                std::cos(theta);

            const double y =
                radius *
                std::sin(theta);

            optics::OpticalRay opticalRay;

            opticalRay.ray =
                optics::Ray(
                    pupilCenter +
                    glm::dvec3(
                        x,
                        y,
                        0.0),
                    rayDirection);

            bundle.push_back(
                opticalRay);
        }

        return bundle;
    }
}