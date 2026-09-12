#include "OrbitCameraController.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/constants.hpp>
#include <glm/gtx/norm.hpp>

OrbitCameraController::OrbitCameraController(Camera& camera)
    : m_camera(camera),
    m_pivot(camera.target())
{
    syncFromCamera();
}

void OrbitCameraController::setPivot(const glm::vec3& pivot)
{
    m_pivot = pivot;
    syncFromCamera();
}

const glm::vec3& OrbitCameraController::pivot() const noexcept
{
    return m_pivot;
}

void OrbitCameraController::setOrbitSensitivity(float radiansPerPixel)
{
    m_orbitSensitivity = radiansPerPixel;
}

void OrbitCameraController::setPanSensitivity(float scale)
{
    m_panSensitivity = scale;
}

void OrbitCameraController::setZoomSensitivity(float scale)
{
    m_zoomSensitivity = scale;
}

void OrbitCameraController::setPitchLimits(
    float minimumPitchRadians,
    float maximumPitchRadians)
{
    m_minPitch = minimumPitchRadians;
    m_maxPitch = maximumPitchRadians;

    m_pitch = std::clamp(
        m_pitch,
        m_minPitch,
        m_maxPitch);

    updateCamera();
}

void OrbitCameraController::setDistanceLimits(
    float minimumDistance,
    float maximumDistance)
{
    m_minDistance = minimumDistance;
    m_maxDistance = maximumDistance;

    m_distance = std::clamp(
        m_distance,
        m_minDistance,
        m_maxDistance);

    updateCamera();
}

void OrbitCameraController::orbit(float deltaX, float deltaY)
{
    m_yaw -= deltaX * m_orbitSensitivity;
    m_pitch += deltaY * m_orbitSensitivity;

    m_pitch = std::clamp(
        m_pitch,
        m_minPitch,
        m_maxPitch);

    updateCamera();
}

void OrbitCameraController::pan(float deltaX, float deltaY)
{
    const glm::vec3 forward =
        glm::normalize(m_pivot - m_camera.position());

    glm::vec3 right =
        glm::cross(forward, m_camera.up());

    if (glm::length2(right) < 1e-12f)
        return;

    right = glm::normalize(right);

    const glm::vec3 up =
        glm::normalize(glm::cross(right, forward));

    /*
     * Scale pan speed with camera distance so trucking feels
     * approximately consistent while zoomed in/out.
     */
    const float scale =
        m_panSensitivity * m_distance;

    const glm::vec3 translation =
        (-deltaX * right + deltaY * up) * scale;

    m_pivot += translation;

    updateCamera();
}

void OrbitCameraController::zoom(float delta)
{
    /*
     * Exponential zoom is generally nicer than subtracting a fixed
     * amount because it behaves consistently over very large scales.
     *
     * Positive delta => smaller distance => zoom in.
     */
    m_distance *= std::exp(
        -delta * m_zoomSensitivity);

    m_distance = std::clamp(
        m_distance,
        m_minDistance,
        m_maxDistance);

    updateCamera();
}

void OrbitCameraController::syncFromCamera()
{
    const glm::vec3 offset =
        m_camera.position() - m_pivot;

    m_distance = glm::length(offset);

    if (m_distance < m_minDistance)
        m_distance = m_minDistance;

    const glm::vec3 direction =
        offset / m_distance;

    /*
     * Coordinate convention:
     *
     * Y = up
     * yaw  = rotation around Y
     * pitch = elevation above XZ plane
     */
    m_pitch = std::asin(
        std::clamp(direction.y, -1.0f, 1.0f));

    m_yaw = std::atan2(
        direction.x,
        direction.z);

    m_pitch = std::clamp(
        m_pitch,
        m_minPitch,
        m_maxPitch);

    updateCamera();
}

void OrbitCameraController::updateCamera()
{
    const float cosPitch = std::cos(m_pitch);

    glm::vec3 offset;

    offset.x =
        m_distance *
        cosPitch *
        std::sin(m_yaw);

    offset.y =
        m_distance *
        std::sin(m_pitch);

    offset.z =
        m_distance *
        cosPitch *
        std::cos(m_yaw);

    m_camera.setPosition(
        m_pivot + offset);

    m_camera.setTarget(
        m_pivot);
}