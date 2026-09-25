// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace opticforge::optics
{

    class Transform
    {
    public:
        Transform();

        explicit Transform(
            const glm::dvec3& position);

        Transform(
            const glm::dvec3& position,
            const glm::dquat& rotation);

        // Position
        const glm::dvec3& position() const;
        void setPosition(const glm::dvec3& position);

        void translate(const glm::dvec3& delta);

        // Rotation
        const glm::dquat& rotation() const;
        void setRotation(const glm::dquat& rotation);

        // Convenience: Euler angles in radians.
        // Rotation convention is GLM quaternion construction convention.
        void setEulerRadians(const glm::dvec3& eulerRadians);
        glm::dvec3 eulerRadians() const;

        // Convenience: Euler angles in degrees.
        void setEulerDegrees(const glm::dvec3& eulerDegrees);
        glm::dvec3 eulerDegrees() const;

        // Incremental rotations.
        void rotateLocal(const glm::dquat& rotation);
        void rotateWorld(const glm::dquat& rotation);

        void rotateLocalRadians(
            double angleRadians,
            const glm::dvec3& localAxis);

        void rotateWorldRadians(
            double angleRadians,
            const glm::dvec3& worldAxis);

        // Transformation matrices.
        glm::dmat4 matrix() const;
        glm::dmat4 inverseMatrix() const;

        // Point transformation.
        glm::dvec3 localToWorldPoint(
            const glm::dvec3& point) const;

        glm::dvec3 worldToLocalPoint(
            const glm::dvec3& point) const;

        // Direction/vector transformation.
        // Translation is intentionally ignored.
        glm::dvec3 localToWorldDirection(
            const glm::dvec3& direction) const;

        glm::dvec3 worldToLocalDirection(
            const glm::dvec3& direction) const;

        // Normal transformation.
        //
        // Because optical Transform is rigid-body only, normals transform
        // exactly like directions. Separate functions make intent explicit
        // and allow implementation changes later if Transform ever evolves.
        glm::dvec3 localToWorldNormal(
            const glm::dvec3& normal) const;

        glm::dvec3 worldToLocalNormal(
            const glm::dvec3& normal) const;

        // Local basis vectors expressed in world coordinates.
        glm::dvec3 right() const;
        glm::dvec3 up() const;
        glm::dvec3 forward() const;

        // Resets to identity transform.
        void reset();

        static Transform identity();

    private:
        glm::dvec3 m_position;
        glm::dquat m_rotation;

        void normalizeRotation();
    };

} // namespace opticforge::optics