#pragma once

#include "Heron/Renderer/Buffer.h"
#include <vulkan/vulkan.h>

namespace Heron {

	class VulkanVertexBuffer : public VertexBuffer {
	public:
		VulkanVertexBuffer(float* vertices, uint32_t size);
		virtual ~VulkanVertexBuffer();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual const BufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

		VkBuffer GetBuffer() const { return m_Buffer; }

	private:
		VkBuffer m_Buffer;
		VkDeviceMemory m_BufferMemory;
		BufferLayout m_Layout;
	};

	class VulkanIndexBuffer : public IndexBuffer {
	public:
		VulkanIndexBuffer(uint32_t* indices, uint32_t count);
		virtual ~VulkanIndexBuffer();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual uint32_t GetCount() const override { return m_Count; }

		VkBuffer GetBuffer() const { return m_Buffer; }

	private:
		VkBuffer m_Buffer;
		VkDeviceMemory m_BufferMemory;
		uint32_t m_Count;
	};

}
