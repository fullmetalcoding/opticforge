#include "Camera.h"
#include <glm/gtx/norm.hpp>
#include <stdexcept>

Camera::Camera(
    const glm::vec3& position,
    const glm::vec3& target,
    const glm::vec3& up)
    : m_position(position),
    m_target(target),
    m_up(glm::normalize(up))
{
}

void Camera::setPosition(const glm::vec3& position)
{
    m_position = position;
}

void Camera::setTarget(const glm::vec3& target)
{
    m_target = target;
}

void Camera::setUp(const glm::vec3& up)
{
    if (glm::length2(up) == 0.0f)
        throw std::invalid_argument("Camera up vector cannot be zero.");

    m_up = glm::normalize(up);
}

void Camera::setPerspective(
    float verticalFovDegrees,
    float nearPlane,
    float farPlane)
{
    if (verticalFovDegrees <= 0.0f ||
        verticalFovDegrees >= 180.0f)
    {
        throw std::invalid_argument(
            "Camera vertical FOV must be between 0 and 180 degrees.");
    }

    if (nearPlane <= 0.0f)
        throw std::invalid_argument("Camera near plane must be positive.");

    if (farPlane <= nearPlane)
        throw std::invalid_argument(
            "Camera far plane must be greater than near plane.");

    m_verticalFovDegrees = verticalFovDegrees;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
}

const glm::vec3& Camera::position() const noexcept
{
    return m_position;
}

const glm::vec3& Camera::target() const noexcept
{
    return m_target;
}

const glm::vec3& Camera::up() const noexcept
{
    return m_up;
}

glm::vec3 Camera::forward() const
{
    return glm::normalize(m_target - m_position);
}

glm::vec3 Camera::right() const
{
    return glm::normalize(glm::cross(forward(), m_up));
}

glm::vec3 Camera::actualUp() const
{
    return glm::normalize(glm::cross(right(), forward()));
}

float Camera::verticalFovDegrees() const noexcept
{
    return m_verticalFovDegrees;
}

float Camera::nearPlane() const noexcept
{
    return m_nearPlane;
}

float Camera::farPlane() const noexcept
{
    return m_farPlane;
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(
        m_position,
        m_target,
        m_up);
}

glm::mat4 Camera::projectionMatrix(float aspectRatio) const
{
    if (aspectRatio <= 0.0f)
        throw std::invalid_argument(
            "Camera aspect ratio must be positive.");

    return glm::perspective(
        glm::radians(m_verticalFovDegrees),
        aspectRatio,
        m_nearPlane,
        m_farPlane);
}