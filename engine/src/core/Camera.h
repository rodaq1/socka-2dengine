#pragma once

#include <glm/glm.hpp>

namespace Engine {

class Camera {
public:
    Camera();
    virtual ~Camera() = default;

    // --- Setters for core properties ---
    void setPosition(const glm::vec2& position) { m_Position = position; recalculateViewMatrix(); }
    void setRotation(float rotation) { m_Rotation = rotation; recalculateViewMatrix(); } // degrees
    void setOrthographicSize(float size) { m_OrthographicSize = size; recalculateProjectionMatrix(); }
    void setAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; recalculateProjectionMatrix(); }
    void setNearClip(float nearClip) { m_NearClip = nearClip; recalculateProjectionMatrix(); }
    void setFarClip(float farClip) { m_FarClip = farClip; recalculateProjectionMatrix(); }

    // --- Getters ---
    const glm::vec2& getPosition() const { return m_Position; }
    float getRotation() const { return m_Rotation; }
    float getOrthographicSize() const { return m_OrthographicSize; }
    float getAspectRatio() const { return m_AspectRatio; }
    float getNearClip() const { return m_NearClip; }
    float getFarClip() const { return m_FarClip; }

    const glm::mat4& getProjectionMatrix() const { return m_ProjectionMatrix; }
    const glm::mat4& getViewMatrix() const { return m_ViewMatrix; }
    const glm::mat4& getViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

private:
    void recalculateViewMatrix();
    void recalculateProjectionMatrix();

private:
    // Transform
    glm::vec2 m_Position = { 0.0f, 0.0f };
    float m_Rotation = 0.0f; 

    float m_OrthographicSize = 100.0f; 
    float m_AspectRatio = 1.778f;
    float m_NearClip = -1.0f;
    float m_FarClip = 1.0f;

    // Matrices
    glm::mat4 m_ProjectionMatrix;
    glm::mat4 m_ViewMatrix;
    glm::mat4 m_ViewProjectionMatrix;
};

} // namespace Engine
