#include "hrnpch.h"
#include "EditorCamera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Heron {

	EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
		: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
	{
		UpdateProjection();
		UpdateView();
	}

	void EditorCamera::UpdateProjection() {
		m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
		m_ViewProjection = m_Projection * m_ViewMatrix;
	}

	void EditorCamera::UpdateView() {
		// Build view matrix from position and orientation (pitch/yaw)
		// DO NOT reset m_Position here — that was destroying pan/zoom!
		glm::quat orientation = GetOrientation();
		m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
		m_ViewMatrix = glm::inverse(m_ViewMatrix);
		
		m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
		m_ViewProjection = m_Projection * m_ViewMatrix;
	}

	void EditorCamera::OnUpdate(float ts) {
		UpdateView();
	}

	void EditorCamera::OnResize(uint32_t width, uint32_t height) {
		if (width == 0 || height == 0) return;
		m_AspectRatio = (float)width / (float)height;
		UpdateProjection();
	}

	void EditorCamera::MousePan(const glm::vec2& delta) {
		float panSpeed = 0.01f; 
		m_Position += -GetRightDirection() * delta.x * panSpeed;
		m_Position += GetUpDirection() * delta.y * panSpeed;
		UpdateView();
	}

	void EditorCamera::MouseRotate(const glm::vec2& delta) {
		float rotationSpeed = 0.003f;
		m_Yaw += delta.x * rotationSpeed;
		m_Pitch += delta.y * rotationSpeed;

		// Clamp pitch so the camera can't flip upside down
		m_Pitch = glm::clamp(m_Pitch, -1.5f, 1.5f);
		UpdateView();
	}

	void EditorCamera::MouseZoom(float delta) {
		float zoomSpeed = 0.5f;
		m_Position += GetForwardDirection() * delta * zoomSpeed;
		UpdateView();
	}

	glm::vec3 EditorCamera::GetUpDirection() const {
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetRightDirection() const {
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetForwardDirection() const {
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::quat EditorCamera::GetOrientation() const {
		return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
	}

}
