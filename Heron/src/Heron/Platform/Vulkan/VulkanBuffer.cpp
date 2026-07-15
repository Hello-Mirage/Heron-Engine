#include "hrnpch.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"

namespace Heron {

	// =========================================================================
	// VulkanVertexBuffer
	// =========================================================================

	VulkanVertexBuffer::VulkanVertexBuffer(float* vertices, uint32_t size)
	{
		VkDevice device = VulkanContext::Get()->GetDevice();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		VulkanContext::Get()->CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data);
		memcpy(data, vertices, (size_t)size);
		vkUnmapMemory(device, stagingBufferMemory);

		VulkanContext::Get()->CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Buffer, m_BufferMemory);

		VulkanContext::Get()->CopyBuffer(stagingBuffer, m_Buffer, size);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		VkDevice device = VulkanContext::Get()->GetDevice();
		vkDestroyBuffer(device, m_Buffer, nullptr);
		vkFreeMemory(device, m_BufferMemory, nullptr);
	}

	void VulkanVertexBuffer::Bind() const
	{
		// Binding happens during command buffer recording in Vulkan
	}

	void VulkanVertexBuffer::Unbind() const
	{
	}

	// =========================================================================
	// VulkanIndexBuffer
	// =========================================================================

	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t* indices, uint32_t count)
		: m_Count(count)
	{
		VkDeviceSize bufferSize = sizeof(uint32_t) * count;
		VkDevice device = VulkanContext::Get()->GetDevice();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		VulkanContext::Get()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices, (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		VulkanContext::Get()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Buffer, m_BufferMemory);

		VulkanContext::Get()->CopyBuffer(stagingBuffer, m_Buffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		VkDevice device = VulkanContext::Get()->GetDevice();
		vkDestroyBuffer(device, m_Buffer, nullptr);
		vkFreeMemory(device, m_BufferMemory, nullptr);
	}

	void VulkanIndexBuffer::Bind() const
	{
	}

	void VulkanIndexBuffer::Unbind() const
	{
	}

}
