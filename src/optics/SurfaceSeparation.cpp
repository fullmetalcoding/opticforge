#include "SurfaceSeparation.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>
#include <variant>

namespace opticforge::optics
{
    namespace
    {
        double sag(
            const SurfaceGeometry& geometry,
            double r)
        {
            return std::visit(
                [r](const auto& g) -> double
                {
                    using T = std::decay_t<decltype(g)>;

                    if constexpr (
                        std::is_same_v<T, PlaneGeometry>)
                    {
                        return 0.0;
                    }
                    else
                    {
                        return g.sag(r);
                    }
                },
                geometry);
        }
    }

    std::optional<double> minimumAxialSeparation(
        const SurfaceGeometry& frontGeometry,
        const SurfaceGeometry& rearGeometry,
        double innerRadius,
        double outerRadius)
    {
        if (!std::isfinite(innerRadius) ||
            !std::isfinite(outerRadius) ||
            innerRadius < 0.0 ||
            outerRadius <= innerRadius)
        {
            return std::nullopt;
        }

        double maximumDifference =
            -std::numeric_limits<double>::infinity();

        const auto consider =
            [&](double r) -> bool
            {
                const double front =
                    sag(frontGeometry, r);

                const double rear =
                    sag(rearGeometry, r);

                if (!std::isfinite(front) ||
                    !std::isfinite(rear))
                {
                    return false;
                }

                maximumDifference =
                    std::max(
                        maximumDifference,
                        front - rear);

                return true;
            };

        //
        // The extrema can always occur at the aperture
        // boundaries.
        //
        if (!consider(innerRadius) ||
            !consider(outerRadius))
        {
            return std::nullopt;
        }

        //
        // For two conics there can be one additional
        // interior extremum where their slopes are equal.
        //
        const auto* frontConic =
            std::get_if<ConicGeometry>(
                &frontGeometry);

        const auto* rearConic =
            std::get_if<ConicGeometry>(
                &rearGeometry);

        if (frontConic && rearConic)
        {
            const double frontR =
                frontConic->radiusOfCurvature();

            const double rearR =
                rearConic->radiusOfCurvature();

            //
            // Conics with opposite curvature signs cannot
            // have equal non-zero radial slopes.
            //
            if (frontR != 0.0 &&
                rearR != 0.0 &&
                std::signbit(frontR) ==
                std::signbit(rearR))
            {
                const double frontQ =
                    1.0 +
                    frontConic->conicConstant();

                const double rearQ =
                    1.0 +
                    rearConic->conicConstant();

                const double denominator =
                    frontQ - rearQ;

                constexpr double epsilon =
                    1.0e-14;

                if (std::abs(denominator) >
                    epsilon)
                {
                    const double rSquared =
                        (
                            frontR * frontR -
                            rearR * rearR
                            ) /
                        denominator;

                    if (rSquared > 0.0 &&
                        std::isfinite(rSquared))
                    {
                        const double r =
                            std::sqrt(rSquared);

                        if (r > innerRadius &&
                            r < outerRadius)
                        {
                            if (!consider(r))
                                return std::nullopt;
                        }
                    }
                }
            }
        }

        return std::max(
            0.0,
            maximumDifference);
    }
}