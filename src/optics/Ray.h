// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <glm/glm.hpp>

namespace opticforge::optics
{

    struct Ray
    {
        glm::dvec3 origin{ 0.0, 0.0, 0.0 };
        glm::dvec3 direction{ 0.0, 0.0, 1.0 };

        Ray() = default;

        Ray(
            const glm::dvec3& origin_,
            const glm::dvec3& direction_)
            : origin(origin_),
            direction(glm::normalize(direction_))
        {
        }

        glm::dvec3 pointAt(double t) const
        {
            return origin + direction * t;
        }
    };

    struct OpticalRay
    {
        Ray ray; 
        double wavelength = 550.0;
        double intensity = 1.0;

    };

} // namespace opticforge::optics