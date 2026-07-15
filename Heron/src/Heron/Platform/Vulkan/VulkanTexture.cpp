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
		// To do
	}

	VulkanTextureCube::~VulkanTextureCube()
	{
	}

	void VulkanTextureCube::Bind(uint32_t slot) const
	{
		VulkanContext::Get()->SetBoundTexture(slot, m_ImageView, m_Sampler);
	}

}
