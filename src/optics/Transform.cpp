// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Transform.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace opticforge::optics
{

    Transform::Transform()
        : m_position(0.0, 0.0, 0.0),
        m_rotation(1.0, 0.0, 0.0, 0.0)
    {
    }

    Transform::Transform(
        const glm::dvec3& position)
        : m_position(position),
        m_rotation(1.0, 0.0, 0.0, 0.0)
    {
    }

    Transform::Transform(
        const glm::dvec3& position,
        const glm::dquat& rotation)
        : m_position(position),
        m_rotation(rotation)
    {
        normalizeRotation();
    }

    const glm::dvec3& Transform::position() const
    {
        return m_position;
    }

    void Transform::setPosition(
        const glm::dvec3& position)
    {
        m_position = position;
    }

    void Transform::translate(
        const glm::dvec3& delta)
    {
        m_position += delta;
    }

    const glm::dquat& Transform::rotation() const
    {
        return m_rotation;
    }

    void Transform::setRotation(
        const glm::dquat& rotation)
    {
        m_rotation = rotation;
        normalizeRotation();
    }

    void Transform::setEulerRadians(
        const glm::dvec3& eulerRadians)
    {
        m_rotation = glm::dquat(eulerRadians);
        normalizeRotation();
    }

    glm::dvec3 Transform::eulerRadians() const
    {
        return glm::eulerAngles(m_rotation);
    }

    void Transform::setEulerDegrees(
        const glm::dvec3& eulerDegrees)
    {
        setEulerRadians(glm::radians(eulerDegrees));
    }

    glm::dvec3 Transform::eulerDegrees() const
    {
        return glm::degrees(eulerRadians());
    }

    void Transform::rotateLocal(
        const glm::dquat& rotation)
    {
        // Post-multiplication applies the incremental rotation
        // in this transform's local coordinate system.
        m_rotation = m_rotation * rotation;
        normalizeRotation();
    }

    void Transform::rotateWorld(
        const glm::dquat& rotation)
    {
        // Pre-multiplication applies the incremental rotation
        // in world coordinates.
        m_rotation = rotation * m_rotation;
        normalizeRotation();
    }

    void Transform::rotateLocalRadians(
        double angleRadians,
        const glm::dvec3& localAxis)
    {
        const double length = glm::length(localAxis);

        if (length == 0.0)
            return;

        const glm::dvec3 axis =
            localAxis / length;

        rotateLocal(
            glm::angleAxis(
                angleRadians,
                axis));
    }

    void Transform::rotateWorldRadians(
        double angleRadians,
        const glm::dvec3& worldAxis)
    {
        const double length = glm::length(worldAxis);

        if (length == 0.0)
            return;

        const glm::dvec3 axis =
            worldAxis / length;

        rotateWorld(
            glm::angleAxis(
                angleRadians,
                axis));
    }

    glm::dmat4 Transform::matrix() const
    {
        const glm::dmat4 translation =
            glm::translate(
                glm::dmat4(1.0),
                m_position);

        const glm::dmat4 rotation =
            glm::mat4_cast(m_rotation);

        return translation * rotation;
    }

    glm::dmat4 Transform::inverseMatrix() const
    {
        // Since this transform contains only rotation and translation,
        // we can construct the inverse analytically rather than using
        // a general-purpose matrix inverse.

        const glm::dquat inverseRotation =
            glm::conjugate(m_rotation);

        const glm::dmat4 rotation =
            glm::mat4_cast(inverseRotation);

        const glm::dmat4 translation =
            glm::translate(
                glm::dmat4(1.0),
                -m_position);

        return rotation * translation;
    }

    glm::dvec3 Transform::localToWorldPoint(
        const glm::dvec3& point) const
    {
        return m_position +
            (m_rotation * point);
    }

    glm::dvec3 Transform::worldToLocalPoint(
        const glm::dvec3& point) const
    {
        const glm::dquat inverseRotation =
            glm::conjugate(m_rotation);

        return inverseRotation *
            (point - m_position);
    }

    glm::dvec3 Transform::localToWorldDirection(
        const glm::dvec3& direction) const
    {
        return m_rotation * direction;
    }

    glm::dvec3 Transform::worldToLocalDirection(
        const glm::dvec3& direction) const
    {
        return glm::conjugate(m_rotation) *
            direction;
    }

    glm::dvec3 Transform::localToWorldNormal(
        const glm::dvec3& normal) const
    {
        return glm::normalize(
            localToWorldDirection(normal));
    }

    glm::dvec3 Transform::worldToLocalNormal(
        const glm::dvec3& normal) const
    {
        return glm::normalize(
            worldToLocalDirection(normal));
    }

    glm::dvec3 Transform::right() const
    {
        return localToWorldDirection(
            glm::dvec3(1.0, 0.0, 0.0));
    }

    glm::dvec3 Transform::up() const
    {
        return localToWorldDirection(
            glm::dvec3(0.0, 1.0, 0.0));
    }

    glm::dvec3 Transform::forward() const
    {
        // Recommended Opticforge convention:
        // optical surfaces have their nominal axis along local +Z.
        return localToWorldDirection(
            glm::dvec3(0.0, 0.0, 1.0));
    }

    void Transform::reset()
    {
        m_position =
            glm::dvec3(0.0, 0.0, 0.0);

        m_rotation =
            glm::dquat(1.0, 0.0, 0.0, 0.0);
    }

    Transform Transform::identity()
    {
        return Transform();
    }

    void Transform::normalizeRotation()
    {
        const double length =
            glm::length(m_rotation);

        if (length == 0.0)
        {
            m_rotation =
                glm::dquat(1.0, 0.0, 0.0, 0.0);

            return;
        }

        m_rotation =
            glm::normalize(m_rotation);
    }

} // namespace opticforge::optics