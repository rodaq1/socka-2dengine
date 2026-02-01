#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine {

Camera::Camera() {
    recalculateProjectionMatrix();
    recalculateViewMatrix();
}

void Camera::recalculateViewMatrix() {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(m_Position.x, m_Position.y, 0.0f)) *
                          glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), glm::vec3(0, 0, 1));

    m_ViewMatrix = glm::inverse(transform);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void Camera::recalculateProjectionMatrix() {
    float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
    float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
    float orthoTop = m_OrthographicSize * 0.5f;
    float orthoBottom = -m_OrthographicSize * 0.5f;

    m_ProjectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_NearClip, m_FarClip);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

} // namespace Engine
