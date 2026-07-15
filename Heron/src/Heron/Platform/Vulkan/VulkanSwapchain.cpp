#include "hrnpch.h"
#include "VulkanSwapchain.h"
#include "VulkanContext.h"

namespace Heron {

	VulkanSwapchain::VulkanSwapchain(VkSurfaceKHR surface, uint32_t width, uint32_t height)
		: m_Surface(surface), m_Width(width), m_Height(height)
	{
		Init(width, height);
	}

	VulkanSwapchain::~VulkanSwapchain()
	{
		VkDevice device = VulkanContext::Get()->GetDevice();
		vkDeviceWaitIdle(device);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device, m_RenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, m_ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, m_InFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(device, m_CommandPool, nullptr);

		for (auto framebuffer : m_Framebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		vkDestroyRenderPass(device, m_RenderPass, nullptr);

		for (auto imageView : m_ImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
	}

	void VulkanSwapchain::Init(uint32_t width, uint32_t height)
	{
		HRN_CORE_INFO("VulkanSwapchain::Init started with width {0}, height {1}", width, height);
		m_Width = width;
		m_Height = height;

		HRN_CORE_INFO("Creating Swapchain...");
		CreateSwapchain();
		HRN_CORE_INFO("Creating Image Views...");
		CreateImageViews();
		HRN_CORE_INFO("Creating Depth Resources...");
		CreateDepthResources();
		HRN_CORE_INFO("Creating Render Pass...");
		CreateRenderPass();
		HRN_CORE_INFO("Creating Framebuffers...");
		CreateFramebuffers();
		HRN_CORE_INFO("Creating Command Pool...");
		CreateCommandPool();
		HRN_CORE_INFO("Allocating Command Buffers...");
		AllocateCommandBuffers();
		HRN_CORE_INFO("Creating Sync Objects...");
		CreateSyncObjects();
		HRN_CORE_INFO("VulkanSwapchain::Init finished!");
	}

	void VulkanSwapchain::OnResize(uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;
		if (m_Width == 0 || m_Height == 0) return;

		VkDevice device = VulkanContext::Get()->GetDevice();
		vkDeviceWaitIdle(device);

		for (auto framebuffer : m_Framebuffers) vkDestroyFramebuffer(device, framebuffer, nullptr);
		for (auto imageView : m_ImageViews) vkDestroyImageView(device, imageView, nullptr);
		m_Framebuffers.clear();
		m_ImageViews.clear();

		vkDestroyImageView(device, m_DepthImageView, nullptr);
		vkDestroyImage(device, m_DepthImage, nullptr);
		vkFreeMemory(device, m_DepthImageMemory, nullptr);

		VkSwapchainKHR oldSwapchain = m_Swapchain;
		CreateSwapchain();
		vkDestroySwapchainKHR(device, oldSwapchain, nullptr);

		CreateImageViews();
		CreateDepthResources();
		CreateFramebuffers();
	}

	bool VulkanSwapchain::BeginFrame()
	{
		if (m_Width == 0 || m_Height == 0) return false;

		VkDevice device = VulkanContext::Get()->GetDevice();

		vkWaitForFences(device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
		
		VkResult result = vkAcquireNextImageKHR(device, m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &m_ImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			HRN_CORE_INFO("VK_ERROR_OUT_OF_DATE_KHR in BeginFrame. Recreating swapchain...");
			OnResize(m_Width, m_Height);
			return false;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			HRN_CORE_ERROR("Failed to acquire swapchain image!");
			return false;
		}

		vkResetFences(device, 1, &m_InFlightFences[m_CurrentFrame]);

		vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrame], 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		vkBeginCommandBuffer(m_CommandBuffers[m_CurrentFrame], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_RenderPass;
		renderPassInfo.framebuffer = m_Framebuffers[m_ImageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = m_Extent;

		VkClearValue clearValues[2]{};
		clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
		clearValues[1].depthStencil = {1.0f, 0};

		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(m_CommandBuffers[m_CurrentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = static_cast<float>(m_Extent.height);
		viewport.width = static_cast<float>(m_Extent.width);
		viewport.height = -static_cast<float>(m_Extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(m_CommandBuffers[m_CurrentFrame], 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = m_Extent;
		vkCmdSetScissor(m_CommandBuffers[m_CurrentFrame], 0, 1, &scissor);

		VulkanContext::Get()->SetActiveCommandBuffer(m_CommandBuffers[m_CurrentFrame]);

		return true;
	}

	void VulkanSwapchain::EndFrame()
	{
		VkDevice device = VulkanContext::Get()->GetDevice();
		VkQueue graphicsQueue = VulkanContext::Get()->GetGraphicsQueue();

		vkCmdEndRenderPass(m_CommandBuffers[m_CurrentFrame]);
		vkEndCommandBuffer(m_CommandBuffers[m_CurrentFrame]);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrame];

		VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]);

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapchains[] = {m_Swapchain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &m_ImageIndex;

		VkResult result = vkQueuePresentKHR(graphicsQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			HRN_CORE_INFO("Swapchain out of date in EndFrame. Recreating...");
			OnResize(m_Width, m_Height);
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			HRN_CORE_ERROR("Failed to present swapchain image!");
		}

		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void VulkanSwapchain::CreateSwapchain()
	{
		// Hardcoded for now. In a real engine, we query surface formats and present modes.
		VkDevice device = VulkanContext::Get()->GetDevice();
		m_ImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
		m_Extent = { m_Width, m_Height };

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;
		createInfo.minImageCount = MAX_FRAMES_IN_FLIGHT;
		createInfo.imageFormat = m_ImageFormat;
		createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		createInfo.imageExtent = m_Extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		createInfo.clipped = VK_TRUE;

		// Note: To make this robust, we should ideally pass oldSwapchain here, but since we are destroying it
		// immediately in OnResize and doing a full device wait idle, VK_NULL_HANDLE is fine for our simple editor setup.
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to create swapchain!");
		}

		uint32_t imageCount;
		vkGetSwapchainImagesKHR(device, m_Swapchain, &imageCount, nullptr);
		m_Images.resize(imageCount);
		vkGetSwapchainImagesKHR(device, m_Swapchain, &imageCount, m_Images.data());
	}

	void VulkanSwapchain::CreateImageViews()
	{
		VkDevice device = VulkanContext::Get()->GetDevice();
		m_ImageViews.resize(m_Images.size());

		for (size_t i = 0; i < m_Images.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_Images[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_ImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &m_ImageViews[i]) != VK_SUCCESS) {
				HRN_CORE_ERROR("Failed to create image views!");
			}
		}
	}

	void VulkanSwapchain::CreateDepthResources()
	{
		VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

		VulkanContext::Get()->CreateImage(m_Extent.width, m_Extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_DepthImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = depthFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(VulkanContext::Get()->GetDevice(), &viewInfo, nullptr, &m_DepthImageView) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to create depth image view!");
		}
	}

	void VulkanSwapchain::CreateRenderPass()
	{
		VkDevice device = VulkanContext::Get()->GetDevice();

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_ImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = VK_FORMAT_D32_SFLOAT;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::vector<VkAttachmentDescription> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to create render pass!");
		}
	}

	void VulkanSwapchain::CreateFramebuffers()
	{
		VkDevice device = VulkanContext::Get()->GetDevice();
		m_Framebuffers.resize(m_ImageViews.size());

		for (size_t i = 0; i < m_ImageViews.size(); i++) {
			VkImageView attachments[] = {
				m_ImageViews[i],
				m_DepthImageView
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = 2;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = m_Extent.width;
			framebufferInfo.height = m_Extent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS) {
				HRN_CORE_ERROR("Failed to create framebuffer!");
			}
		}
	}

	void VulkanSwapchain::CreateCommandPool()
	{
		VkDevice device = VulkanContext::Get()->GetDevice();

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = 0; // Assuming 0 for now

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to create command pool!");
		}
	}

	void VulkanSwapchain::AllocateCommandBuffers()
	{
		VkDevice device = VulkanContext::Get()->GetDevice();
		m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to allocate command buffers!");
		}
	}

	void VulkanSwapchain::CreateSyncObjects()
	{
		VkDevice device = VulkanContext::Get()->GetDevice();
		m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {
				HRN_CORE_ERROR("Failed to create synchronization objects for a frame!");
			}
		}
	}

}
