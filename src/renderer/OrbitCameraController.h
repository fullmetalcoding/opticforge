// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include "camera.h"

#include <glm/glm.hpp>

class OrbitCameraController
{
public:
    explicit OrbitCameraController(Camera& camera);

    void setPivot(const glm::vec3& pivot);
    [[nodiscard]] const glm::vec3& pivot() const noexcept;

    void setOrbitSensitivity(float radiansPerPixel);
    void setPanSensitivity(float scale);
    void setZoomSensitivity(float scale);

    void setPitchLimits(
        float minimumPitchRadians,
        float maximumPitchRadians);

    void setDistanceLimits(
        float minimumDistance,
        float maximumDistance);

    void orbit(float deltaX, float deltaY);

    // Truck/pan parallel to the camera's image plane.
    void pan(float deltaX, float deltaY);

    // Positive delta zooms in by default.
    void zoom(float delta);

    // Reconstruct yaw/pitch/distance from the current camera position.
    void syncFromCamera();

    // Applies the controller's current state to the Camera.
    void updateCamera();

private:
    Camera& m_camera;

    glm::vec3 m_pivot{ 0.0f };

    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_distance = 1.0f;

    float m_orbitSensitivity = 0.005f;
    float m_panSensitivity = 0.0015f;
    float m_zoomSensitivity = 0.10f;

    float m_minPitch = -1.553343f; // ~ -89 degrees
    float m_maxPitch = 1.553343f;  // ~ +89 degrees

    float m_minDistance = 0.01f;
    float m_maxDistance = 100000.0f;
};
