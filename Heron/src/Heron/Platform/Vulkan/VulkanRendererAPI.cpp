#include "hrnpch.h"
#include "VulkanRendererAPI.h"
#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include <GLFW/glfw3.h>

namespace Heron {

	void VulkanRendererAPI::Init()
	{
		HRN_CORE_INFO("VulkanRendererAPI::Init");
		
		// If GLFW hasn't been initialized by the windowing system (e.g. when using Qt),
		// we must initialize it so glfwGetRequiredInstanceExtensions works.
		if (!glfwInit()) {
			HRN_CORE_ERROR("Failed to initialize GLFW for Vulkan instance extensions!");
		}

		auto context = new VulkanContext(nullptr);
		context->Init();
	}

	void VulkanRendererAPI::SetClearColor(float r, float g, float b, float a)
	{
		// In Vulkan, clear color is usually set during the Render Pass begin info,
		// not globally. We will just store it here or handle it differently.
	}

	void VulkanRendererAPI::Clear()
	{
		// In Vulkan, clearing is done via vkCmdClearAttachments or implicitly during
		// render pass begin. We leave this empty.
	}

	void VulkanRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray)
	{
		VkCommandBuffer cmdBuffer = VulkanContext::Get()->GetActiveCommandBuffer();
		if (cmdBuffer == VK_NULL_HANDLE) {
			HRN_CORE_ERROR("DrawIndexed called outside of an active RenderPass!");
			return;
		}

		// Bind vertex buffers
		const auto& vertexBuffers = vertexArray->GetVertexBuffers();
		std::vector<VkBuffer> vkBuffers;
		std::vector<VkDeviceSize> offsets;
		for (const auto& vb : vertexBuffers) {
			auto vulkanVB = std::static_pointer_cast<VulkanVertexBuffer>(vb);
			vkBuffers.push_back(vulkanVB->GetBuffer());
			offsets.push_back(0);
		}

		vkCmdBindVertexBuffers(cmdBuffer, 0, (uint32_t)vkBuffers.size(), vkBuffers.data(), offsets.data());

		// Bind index buffer
		auto ib = vertexArray->GetIndexBuffer();
		auto vulkanIB = std::static_pointer_cast<VulkanIndexBuffer>(ib);
		vkCmdBindIndexBuffer(cmdBuffer, vulkanIB->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		// Draw
		vkCmdDrawIndexed(cmdBuffer, ib->GetCount(), 1, 0, 0, 0);
	}

	void VulkanRendererAPI::DrawLines(const Ref<VertexArray>& vertexArray)
	{
		// DrawLines is identical to DrawIndexed in Vulkan, because the primitive topology
		// (e.g. VK_PRIMITIVE_TOPOLOGY_LINE_LIST) is baked into the bound pipeline.
		auto cmdBuffer = VulkanContext::Get()->GetActiveCommandBuffer();
		if (!cmdBuffer) return;

		auto& vertexBuffers = vertexArray->GetVertexBuffers();
		std::vector<VkBuffer> vkBuffers;
		std::vector<VkDeviceSize> offsets;

		for (auto& vb : vertexBuffers) {
			auto vulkanVB = std::static_pointer_cast<VulkanVertexBuffer>(vb);
			vkBuffers.push_back(vulkanVB->GetBuffer());
			offsets.push_back(0);
		}

		vkCmdBindVertexBuffers(cmdBuffer, 0, (uint32_t)vkBuffers.size(), vkBuffers.data(), offsets.data());

		auto ib = vertexArray->GetIndexBuffer();
		auto vulkanIB = std::static_pointer_cast<VulkanIndexBuffer>(ib);
		vkCmdBindIndexBuffer(cmdBuffer, vulkanIB->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(cmdBuffer, ib->GetCount(), 1, 0, 0, 0);
	}

	void VulkanRendererAPI::DrawLineLoop(const Ref<VertexArray>& vertexArray)
	{
		// To do
	}

}
