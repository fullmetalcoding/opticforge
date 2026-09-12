#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    Camera() = default;

    Camera(
        const glm::vec3& position,
        const glm::vec3& target,
        const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

    void setPosition(const glm::vec3& position);
    void setTarget(const glm::vec3& target);
    void setUp(const glm::vec3& up);

    void setPerspective(
        float verticalFovDegrees,
        float nearPlane,
        float farPlane);

    [[nodiscard]] const glm::vec3& position() const noexcept;
    [[nodiscard]] const glm::vec3& target() const noexcept;
    [[nodiscard]] const glm::vec3& up() const noexcept;

    [[nodiscard]] glm::vec3 forward() const;
    [[nodiscard]] glm::vec3 right() const;
    [[nodiscard]] glm::vec3 actualUp() const;

    [[nodiscard]] float verticalFovDegrees() const noexcept;
    [[nodiscard]] float nearPlane() const noexcept;
    [[nodiscard]] float farPlane() const noexcept;

    [[nodiscard]] glm::mat4 viewMatrix() const;
    [[nodiscard]] glm::mat4 projectionMatrix(float aspectRatio) const;

private:
    glm::vec3 m_position{ 0.0f, 0.0f, 5.0f };
    glm::vec3 m_target{ 0.0f, 0.0f, 0.0f };
    glm::vec3 m_up{ 0.0f, 1.0f, 0.0f };

    float m_verticalFovDegrees = 45.0f;
    float m_nearPlane = 0.01f;
    float m_farPlane = 10000.0f;
};