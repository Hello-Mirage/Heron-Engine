#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Heron {

	class EditorCamera {
	public:
		EditorCamera() { UpdateView(); UpdateProjection(); }
		EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

		void OnUpdate(float ts);
		void OnResize(uint32_t width, uint32_t height);

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetProjection() const { return m_Projection; }
		const glm::mat4& GetViewProjection() const { return m_ViewProjection; }

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;
		const glm::vec3& GetPosition() const { return m_Position; }
		glm::quat GetOrientation() const;

		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }

	private:
		void UpdateProjection();
		void UpdateView();

	private:
		float m_FOV = 45.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

		glm::mat4 m_ViewMatrix{1.0f};
		glm::mat4 m_Projection{1.0f};
		glm::mat4 m_ViewProjection{1.0f};

		glm::vec3 m_Position = { 0.0f, 2.0f, 10.0f };
		float m_Pitch = 0.5f, m_Yaw = 0.0f; // Pitch down ~30 degrees
	};

}
