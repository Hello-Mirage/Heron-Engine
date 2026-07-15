#pragma once

#include "Heron/Renderer/Shader.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "Heron/Renderer/Buffer.h"

namespace Heron {

	class VulkanShader : public Shader {
	public:
		VulkanShader(const std::string& vertexSrc, const std::string& fragmentSrc);
		virtual ~VulkanShader();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void SetMat4(const std::string& name, const glm::mat4& value) override;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
		virtual void SetFloat(const std::string& name, float value) override;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
		virtual void SetInt(const std::string& name, int value) override;

		// Vulkan specific method to build the pipeline
		void BuildPipeline(VkRenderPass renderPass, const Heron::BufferLayout& layout, bool depthTest = true, bool isWireframe = false, VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		VkPipeline GetPipeline() const { return m_Pipeline; }
		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
		VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }

		struct SceneData {
			glm::mat4 ViewProjection;
			glm::vec4 LightDir; // vec4 to match std140 alignment for vec3
			glm::vec4 LightColor; // vec4 to match std140 alignment for vec3
			float LightIntensity;
			int HasLight;
			float padding[2];
		};

		// This buffer holds the push constant data before binding
		struct PushConstantData {
			glm::mat4 Model;
			glm::vec4 Color;
		} m_PushConstants;

		SceneData m_SceneData;
		bool m_SceneDataDirty = false;

		void PushConstants(VkCommandBuffer cmdBuffer) const;
		void SetSceneData(const SceneData& data);

	private:
		VkShaderModule m_VertexModule = VK_NULL_HANDLE;
		VkShaderModule m_FragmentModule = VK_NULL_HANDLE;

		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;

		VkBuffer m_UniformBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_UniformBufferMemory = VK_NULL_HANDLE;

		mutable VkImageView m_LastImageView = VK_NULL_HANDLE;
		mutable VkSampler m_LastSampler = VK_NULL_HANDLE;

		bool m_IsPipelineBuilt = false;
	};

}
