#include "hrnpch.h"
#include "Shader.h"

#include "Heron/Platform/OpenGL/OpenGLShader.h"

namespace Heron {

	Ref<Shader> Shader::Create(const std::string& vertexSrc, const std::string& fragmentSrc) {
		return CreateRef<OpenGLShader>(vertexSrc, fragmentSrc);
	}

}
