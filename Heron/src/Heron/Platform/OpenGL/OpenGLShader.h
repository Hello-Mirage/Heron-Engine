#pragma once

#include "Heron/Renderer/Shader.h"

namespace Heron {

	class OpenGLShader : public Shader {
	public:
		OpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc);
		virtual ~OpenGLShader();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void SetMat4(const std::string& name, const glm::mat4& value) override;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
		virtual void SetFloat(const std::string& name, float value) override;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;

	private:
		uint32_t m_RendererID;
	};

}
