#pragma once

#include "Heron/Renderer/Texture.h"
#include <vulkan/vulkan.h>

namespace Heron {

	class VulkanTexture2D : public Texture2D {
	public:
		VulkanTexture2D(uint32_t width, uint32_t height);
		VulkanTexture2D(const std::string& path);
		virtual ~VulkanTexture2D();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void Bind(uint32_t slot = 0) const override;

		virtual bool operator==(const Texture& other) const override {
			return false; // To do
		}

		VkImageView GetImageView() const { return m_ImageView; }
		VkSampler GetSampler() const { return m_Sampler; }

	private:
		std::string m_Path;
		uint32_t m_Width, m_Height;
		
		VkImage m_Image;
		VkDeviceMemory m_ImageMemory;
		VkImageView m_ImageView;
		VkSampler m_Sampler;
	};

	class VulkanTextureCube : public TextureCube {
	public:
		VulkanTextureCube(const std::vector<std::string>& paths);
		virtual ~VulkanTextureCube();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void Bind(uint32_t slot = 0) const override;

		virtual bool operator==(const Texture& other) const override {
			return false; // To do
		}

	private:
		uint32_t m_Width, m_Height;
		
		VkImage m_Image;
		VkDeviceMemory m_ImageMemory;
		VkImageView m_ImageView;
		VkSampler m_Sampler;
	};

}
