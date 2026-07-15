#include "hrnpch.h"
#include "Buffer.h"
#include "RendererAPI.h"

#include "Heron/Platform/OpenGL/OpenGLBuffer.h"
#include "Heron/Platform/Vulkan/VulkanBuffer.h"

namespace Heron {

	Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size) {
		switch (RendererAPI::GetAPI()) {
			case RendererAPI::API::None:    HRN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLVertexBuffer>(vertices, size);
			case RendererAPI::API::Vulkan:  return CreateRef<VulkanVertexBuffer>(vertices, size);
		}
		HRN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count) {
		switch (RendererAPI::GetAPI()) {
			case RendererAPI::API::None:    HRN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLIndexBuffer>(indices, count);
			case RendererAPI::API::Vulkan:  return CreateRef<VulkanIndexBuffer>(indices, count);
		}
		HRN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
