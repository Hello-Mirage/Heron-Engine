#include "hrnpch.h"
#include "VulkanTexture.h"
#include "VulkanContext.h"
#include <stb_image.h>

namespace Heron {

	VulkanTexture2D::VulkanTexture2D(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height)
	{
		HRN_CORE_INFO("VulkanTexture2D::VulkanTexture2D creating 1x1 white texture...");
		VkDeviceSize imageSize = m_Width * m_Height * 4;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		HRN_CORE_INFO("Creating staging buffer...");
		VulkanContext::Get()->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		HRN_CORE_INFO("Mapping memory...");
		void* mappedData;
		vkMapMemory(VulkanContext::Get()->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &mappedData);
		uint32_t whitePixel = 0xffffffff;
		memcpy(mappedData, &whitePixel, sizeof(uint32_t));
		vkUnmapMemory(VulkanContext::Get()->GetDevice(), stagingBufferMemory);

		HRN_CORE_INFO("Creating Image...");
		VulkanContext::Get()->CreateImage(m_Width, m_Height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_ImageMemory);

		HRN_CORE_INFO("Transitioning Layout to Transfer Dst...");
		VulkanContext::Get()->TransitionImageLayout(m_Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		HRN_CORE_INFO("Copying Buffer to Image...");
		VulkanContext::Get()->CopyBufferToImage(stagingBuffer, m_Image, m_Width, m_Height);
		HRN_CORE_INFO("Transitioning Layout to Shader Read Only...");
		VulkanContext::Get()->TransitionImageLayout(m_Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		HRN_CORE_INFO("Destroying staging buffer...");
		vkDestroyBuffer(VulkanContext::Get()->GetDevice(), stagingBuffer, nullptr);
		vkFreeMemory(VulkanContext::Get()->GetDevice(), stagingBufferMemory, nullptr);
        HRN_CORE_INFO("VulkanTexture2D created successfully.");

		m_ImageView = VulkanContext::Get()->CreateImageView(m_Image, VK_FORMAT_R8G8B8A8_SRGB);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		if (vkCreateSampler(VulkanContext::Get()->GetDevice(), &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS) {
			HRN_CORE_ASSERT(false, "Failed to create texture sampler!");
		}
	}

	VulkanTexture2D::VulkanTexture2D(const std::string& path)
		: m_Path(path)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
		
		if (data) {
			m_Width = width;
			m_Height = height;
			VkDeviceSize imageSize = m_Width * m_Height * 4;

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VulkanContext::Get()->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			void* mappedData;
			vkMapMemory(VulkanContext::Get()->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &mappedData);
			memcpy(mappedData, data, static_cast<size_t>(imageSize));
			vkUnmapMemory(VulkanContext::Get()->GetDevice(), stagingBufferMemory);

			stbi_image_free(data);

			VulkanContext::Get()->CreateImage(m_Width, m_Height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_ImageMemory);

			VulkanContext::Get()->TransitionImageLayout(m_Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			VulkanContext::Get()->CopyBufferToImage(stagingBuffer, m_Image, m_Width, m_Height);
			VulkanContext::Get()->TransitionImageLayout(m_Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			vkDestroyBuffer(VulkanContext::Get()->GetDevice(), stagingBuffer, nullptr);
			vkFreeMemory(VulkanContext::Get()->GetDevice(), stagingBufferMemory, nullptr);

			m_ImageView = VulkanContext::Get()->CreateImageView(m_Image, VK_FORMAT_R8G8B8A8_SRGB);

			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.anisotropyEnable = VK_FALSE;
			samplerInfo.maxAnisotropy = 1.0f;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 0.0f;

			if (vkCreateSampler(VulkanContext::Get()->GetDevice(), &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS) {
				HRN_CORE_ASSERT(false, "Failed to create texture sampler!");
			}

		} else {
			HRN_CORE_ERROR("Failed to load image: {0}", path);
		}
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		VkDevice device = VulkanContext::Get()->GetDevice();
		if (m_Sampler) vkDestroySampler(device, m_Sampler, nullptr);
		if (m_ImageView) vkDestroyImageView(device, m_ImageView, nullptr);
		if (m_Image) vkDestroyImage(device, m_Image, nullptr);
		if (m_ImageMemory) vkFreeMemory(device, m_ImageMemory, nullptr);
	}

	void VulkanTexture2D::Bind(uint32_t slot) const
	{
		VulkanContext::Get()->SetBoundTexture(slot, m_ImageView, m_Sampler);
	}

	VulkanTextureCube::VulkanTextureCube(const std::vector<std::string>& paths)
	{
		HRN_CORE_INFO("Entering VulkanTextureCube constructor...");
		HRN_CORE_ASSERT(paths.size() == 6, "VulkanTextureCube requires exactly 6 paths!");
		
		int width, height, channels;
		stbi_set_flip_vertically_on_load(0);
		
		HRN_CORE_INFO("Loading first face: {0}", paths[0]);
		// Load first face to get dimensions
		stbi_uc* data0 = stbi_load(paths[0].c_str(), &width, &height, &channels, 4);
		if (!data0) {
			HRN_CORE_ERROR("Failed to load cubemap face: {0}", paths[0]);
			return;
		}
		
		m_Width = width;
		m_Height = height;
		VkDeviceSize layerSize = m_Width * m_Height * 4;
		VkDeviceSize imageSize = layerSize * 6;
		HRN_CORE_INFO("Face 0 loaded successfully. Width: {0}, Height: {1}, ImageSize: {2}", m_Width, m_Height, imageSize);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		HRN_CORE_INFO("Creating staging buffer...");
		VulkanContext::Get()->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		HRN_CORE_INFO("Mapping staging buffer memory...");
		void* mappedData;
		vkMapMemory(VulkanContext::Get()->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &mappedData);
		
		// Copy face 0
		memcpy(mappedData, data0, static_cast<size_t>(layerSize));
		stbi_image_free(data0);

		HRN_CORE_INFO("Loading remaining 5 faces...");
		// Load and copy faces 1-5
		for (int i = 1; i < 6; i++) {
			int w, h, c;
			stbi_uc* data = stbi_load(paths[i].c_str(), &w, &h, &c, 4);
			if (data && w == m_Width && h == m_Height) {
				memcpy(static_cast<char*>(mappedData) + (layerSize * i), data, static_cast<size_t>(layerSize));
				stbi_image_free(data);
			} else {
				if (data) stbi_image_free(data);
				HRN_CORE_ERROR("Failed to load cubemap face {0} or size mismatch: {1}", i, paths[i]);
			}
		}
		vkUnmapMemory(VulkanContext::Get()->GetDevice(), stagingBufferMemory);

		HRN_CORE_INFO("Creating CUBE_COMPATIBLE Vulkan Image...");
		// Create Image (CUBE_COMPATIBLE)
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = m_Width;
		imageInfo.extent.height = m_Height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 6;
		imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		VkDevice device = VulkanContext::Get()->GetDevice();
		if (vkCreateImage(device, &imageInfo, nullptr, &m_Image) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to create cubemap image!");
		}

		HRN_CORE_INFO("Allocating cubemap image memory...");
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, m_Image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = VulkanContext::Get()->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &m_ImageMemory) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to allocate cubemap image memory!");
		}
		vkBindImageMemory(device, m_Image, m_ImageMemory, 0);

		HRN_CORE_INFO("Transitioning cubemap layout to TRANSFER_DST...");
		// Transition Layout to Transfer Dst
		VkCommandBuffer commandBuffer = VulkanContext::Get()->BeginSingleTimeCommands();
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_Image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 6;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		
		HRN_CORE_INFO("Copying staging buffer to cubemap image...");
		// Copy Buffer to Image
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 6;
		region.imageOffset = {0, 0, 0};
		region.imageExtent = { m_Width, m_Height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		HRN_CORE_INFO("Transitioning cubemap layout to SHADER_READ_ONLY...");
		// Transition Layout to Shader Read Only
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		VulkanContext::Get()->EndSingleTimeCommands(commandBuffer);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);

		HRN_CORE_INFO("Creating cubemap image view...");
		// Create Image View
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_Image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 6;

		if (vkCreateImageView(device, &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to create cubemap image view!");
		}

		HRN_CORE_INFO("Creating cubemap sampler...");
		// Create Sampler
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to create cubemap sampler!");
		}
		HRN_CORE_INFO("VulkanTextureCube constructor finished successfully!");
	}

	VulkanTextureCube::~VulkanTextureCube()
	{
		VkDevice device = VulkanContext::Get()->GetDevice();
		if (m_Sampler) vkDestroySampler(device, m_Sampler, nullptr);
		if (m_ImageView) vkDestroyImageView(device, m_ImageView, nullptr);
		if (m_Image) vkDestroyImage(device, m_Image, nullptr);
		if (m_ImageMemory) vkFreeMemory(device, m_ImageMemory, nullptr);
	}

	void VulkanTextureCube::Bind(uint32_t slot) const
	{
		if (m_ImageView != VK_NULL_HANDLE && m_Sampler != VK_NULL_HANDLE) {
			VulkanContext::Get()->SetBoundTexture(slot, m_ImageView, m_Sampler);
		} else {
			// Fallback: If creation failed, bind nothing or bind a default.
			// VulkanContext will handle VK_NULL_HANDLE if we pass it, but to prevent crash 
			// it's safer to not bind or just pass null and let the renderer handle it safely.
			VulkanContext::Get()->SetBoundTexture(slot, VK_NULL_HANDLE, VK_NULL_HANDLE);
		}
	}

}
