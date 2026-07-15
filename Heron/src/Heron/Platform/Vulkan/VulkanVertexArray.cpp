#include "hrnpch.h"
#include "VulkanVertexArray.h"

namespace Heron {

	VulkanVertexArray::VulkanVertexArray()
	{
	}

	VulkanVertexArray::~VulkanVertexArray()
	{
	}

	void VulkanVertexArray::Bind() const
	{
		// Vulkan doesn't have Vertex Arrays objects like OpenGL.
		// The binding happens during vkCmdBindVertexBuffers and vkCmdBindIndexBuffer
	}

	void VulkanVertexArray::Unbind() const
	{
	}

	void VulkanVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
	{
		HRN_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");
		m_VertexBuffers.push_back(vertexBuffer);
	}

	void VulkanVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
	{
		m_IndexBuffer = indexBuffer;
	}

}
