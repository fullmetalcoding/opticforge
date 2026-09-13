#include "ConicGeometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace opticforge::optics
{

    ConicGeometry::ConicGeometry(
        double radiusOfCurvature,
        double conicConstant)
        : m_radiusOfCurvature(radiusOfCurvature),
        m_conicConstant(conicConstant)
    {
    }

    double ConicGeometry::radiusOfCurvature() const
    {
        return m_radiusOfCurvature;
    }

    void ConicGeometry::setRadiusOfCurvature(
        double radiusOfCurvature)
    {
        m_radiusOfCurvature =
            radiusOfCurvature;
    }

    double ConicGeometry::conicConstant() const
    {
        return m_conicConstant;
    }

    void ConicGeometry::setConicConstant(
        double conicConstant)
    {
        m_conicConstant =
            conicConstant;
    }

    bool ConicGeometry::intersect(
        const Ray& localRay,
        SurfaceHit& localHit) const
    {
        constexpr double epsilon = 1.0e-12;

        const glm::dvec3& o =
            localRay.origin;

        const glm::dvec3& d =
            localRay.direction;

        const double q =
            1.0 + m_conicConstant;

        // Implicit surface:
        //
        // x² + y² + q z² - 2 R z = 0
        //
        // Substitute:
        //
        // P(t) = O + tD
        //
        // to obtain:
        //
        // A t² + B t + C = 0

        const double A =
            d.x * d.x +
            d.y * d.y +
            q * d.z * d.z;

        const double B =
            2.0 * (
                o.x * d.x +
                o.y * d.y +
                q * o.z * d.z -
                m_radiusOfCurvature * d.z);

        const double C =
            o.x * o.x +
            o.y * o.y +
            q * o.z * o.z -
            2.0 * m_radiusOfCurvature * o.z;

        double t = 0.0;

        if (std::abs(A) < epsilon)
        {
            // Degenerate case: the quadratic reduces to a linear equation.
            if (std::abs(B) < epsilon)
                return false;

            const double candidate =
                -C / B;

            if (candidate <= epsilon)
                return false;

            t = candidate;
        }
        else
        {
            const double discriminant =
                B * B -
                4.0 * A * C;

            if (discriminant < 0.0)
                return false;

            const double sqrtDiscriminant =
                std::sqrt(std::max(
                    0.0,
                    discriminant));

            // Numerically stable quadratic solution.
            const double qRoot =
                -0.5 * (
                    B +
                    std::copysign(
                        sqrtDiscriminant,
                        B));

            double t0;
            double t1;

            if (std::abs(qRoot) < epsilon)
            {
                // Double root.
                t0 =
                    -B /
                    (2.0 * A);

                t1 = t0;
            }
            else
            {
                t0 =
                    qRoot / A;

                t1 =
                    C / qRoot;
            }

            if (t0 > t1)
                std::swap(t0, t1);

            // Choose the closest intersection in front of the ray.
            if (t0 > epsilon)
            {
                t = t0;
            }
            else if (t1 > epsilon)
            {
                t = t1;
            }
            else
            {
                return false;
            }
        }

        localHit.t = t;

        localHit.position =
            localRay.pointAt(t);

        localHit.normal =
            normalAt(localHit.position);

        return true;
    }

    glm::dvec3 ConicGeometry::normalAt(
        const glm::dvec3& localPoint) const
    {
        // Gradient of:
        //
        // F(x,y,z) =
        //     x² + y² + (1+k)z² - 2Rz
        //
        // is:
        //
        // ∇F =
        //     (2x,
        //      2y,
        //      2(1+k)z - 2R)
        //
        // Constant factor 2 can be discarded.

        glm::dvec3 normal(
            localPoint.x,
            localPoint.y,
            (1.0 + m_conicConstant) *
            localPoint.z -
            m_radiusOfCurvature);

        const double length =
            glm::length(normal);

        if (length == 0.0)
        {
            // This should not occur for a valid conic surface point,
            // but provide a deterministic fallback.
            return glm::dvec3(
                0.0,
                0.0,
                1.0);
        }
        // Use a consistent normal orientation: +Z at the surface vertex.
        // The raw implicit gradient reverses when the radius changes sign.
        if (m_radiusOfCurvature > 0.0)
        {
            normal = -normal;
        }

        return normal / length;

    }

    double ConicGeometry::sag(
        double r) const
    {
        constexpr double epsilon = 1.0e-15;

        const double R =
            m_radiusOfCurvature;

        if (std::abs(R) < epsilon)
            return 0.0;

        const double r2 =
            r * r;

        const double argument =
            R * R -
            (1.0 + m_conicConstant) *
            r2;

        if (argument < 0.0)
        {
            // Outside the real domain of this conic branch.
            return std::numeric_limits<double>::quiet_NaN();
        }

        // Equivalent to the common sag equation, but written in a form
        // that preserves the sign convention of R:
        //
        // z = r² /
        //     (R + sign(R) *
        //          sqrt(R² - (1+k)r²))
        //
        // This selects the branch passing through the vertex at z = 0.

        const double denominator =
            R +
            std::copysign(
                std::sqrt(argument),
                R);

        if (std::abs(denominator) < epsilon)
            return 0.0;

        return r2 / denominator;
    }

} // namespace opticforge::optics