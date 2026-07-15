#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "Heron/Core/Core.h"

namespace Heron {

	class VulkanSwapchain {
	public:
		VulkanSwapchain(VkSurfaceKHR surface, uint32_t width, uint32_t height);
		~VulkanSwapchain();

		void Init(uint32_t width, uint32_t height);
		void OnResize(uint32_t width, uint32_t height);

		bool BeginFrame();
		void EndFrame();

		VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
		VkRenderPass GetRenderPass() const { return m_RenderPass; }
		VkFramebuffer GetFramebuffer(uint32_t index) const { return m_Framebuffers[index]; }
		VkCommandBuffer GetCurrentCommandBuffer() const { return m_CommandBuffers[m_CurrentFrame]; }
		uint32_t GetImageCount() const { return (uint32_t)m_Images.size(); }
		uint32_t GetCurrentImageIndex() const { return m_ImageIndex; }

	private:
		void CreateSwapchain();
		void CreateImageViews();
		void CreateDepthResources();
		void CreateRenderPass();
		void CreateFramebuffers();
		void CreateSyncObjects();
		void CreateCommandPool();
		void AllocateCommandBuffers();

	private:
		VkSurfaceKHR m_Surface;
		uint32_t m_Width, m_Height;

		VkSwapchainKHR m_Swapchain;
		VkFormat m_ImageFormat;
		VkExtent2D m_Extent;

		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;
		std::vector<VkFramebuffer> m_Framebuffers;

		VkImage m_DepthImage;
		VkDeviceMemory m_DepthImageMemory;
		VkImageView m_DepthImageView;

		VkRenderPass m_RenderPass;

		VkCommandPool m_CommandPool;
		std::vector<VkCommandBuffer> m_CommandBuffers;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;

		uint32_t m_CurrentFrame = 0;
		uint32_t m_ImageIndex = 0;
		const int MAX_FRAMES_IN_FLIGHT = 2;
	};

}
