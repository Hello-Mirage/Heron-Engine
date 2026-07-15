#pragma once

#include "Heron/Renderer/GraphicsContext.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "Heron/Core/Core.h"
#include <map>

namespace Heron {

	class VulkanContext : public GraphicsContext {
	public:
		VulkanContext(GLFWwindow* windowHandle);
		~VulkanContext() override;

		void Init() override;
		void SwapBuffers() override;

		VkInstance GetInstance() const { return m_Instance; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkDevice GetDevice() const { return m_Device; }
		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }

		VkCommandBuffer GetActiveCommandBuffer() const { return m_ActiveCommandBuffer; }
		void SetActiveCommandBuffer(VkCommandBuffer cmd) { m_ActiveCommandBuffer = cmd; }

		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		VkCommandBuffer BeginSingleTimeCommands();
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		VkImageView CreateImageView(VkImage image, VkFormat format);

		static VulkanContext* Get();

		void SetBoundTexture(uint32_t slot, VkImageView imageView, VkSampler sampler) {
			m_BoundImageViews[slot] = imageView;
			m_BoundSamplers[slot] = sampler;
		}

		VkImageView GetBoundImageView(uint32_t slot) const {
			auto it = m_BoundImageViews.find(slot);
			return it != m_BoundImageViews.end() ? it->second : VK_NULL_HANDLE;
		}

		VkSampler GetBoundSampler(uint32_t slot) const {
			auto it = m_BoundSamplers.find(slot);
			return it != m_BoundSamplers.end() ? it->second : VK_NULL_HANDLE;
		}

	private:
		std::map<uint32_t, VkImageView> m_BoundImageViews;
		std::map<uint32_t, VkSampler> m_BoundSamplers;

		GLFWwindow* m_WindowHandle;
		VkInstance m_Instance;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device;
		VkQueue m_GraphicsQueue;
		VkDescriptorPool m_DescriptorPool;
		VkCommandPool m_TransferCommandPool = VK_NULL_HANDLE;
		VkCommandBuffer m_ActiveCommandBuffer = VK_NULL_HANDLE;

		static VulkanContext* s_Instance;
	};

}
