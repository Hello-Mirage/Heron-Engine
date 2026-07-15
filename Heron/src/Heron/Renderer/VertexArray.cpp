#include "hrnpch.h"
#include "VertexArray.h"
#include "RendererAPI.h"

#include "Heron/Platform/OpenGL/OpenGLVertexArray.h"
#include "Heron/Platform/Vulkan/VulkanVertexArray.h"

namespace Heron {

	Ref<VertexArray> VertexArray::Create() {
		switch (RendererAPI::GetAPI()) {
			case RendererAPI::API::None:    HRN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLVertexArray>();
			case RendererAPI::API::Vulkan:  return CreateRef<VulkanVertexArray>();
		}

		HRN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
