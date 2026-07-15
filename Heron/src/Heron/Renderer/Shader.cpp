#include "hrnpch.h"
#include "Shader.h"
#include "RendererAPI.h"

#include "Heron/Platform/OpenGL/OpenGLShader.h"
#include "Heron/Platform/Vulkan/VulkanShader.h"

namespace Heron {

	Ref<Shader> Shader::Create(const std::string& vertexSrc, const std::string& fragmentSrc) {
		switch (RendererAPI::GetAPI()) {
			case RendererAPI::API::None:    HRN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShader>(vertexSrc, fragmentSrc);
			case RendererAPI::API::Vulkan:  return CreateRef<VulkanShader>(vertexSrc, fragmentSrc);
		}
		HRN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
