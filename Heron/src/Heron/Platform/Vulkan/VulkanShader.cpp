#include "hrnpch.h"
#include "VulkanShader.h"
#include "VulkanContext.h"
#include <fstream>

namespace Heron {

	static std::vector<char> ReadFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			// Fallback: check if we are in the root directory and need to prepend Heron/
			std::string fallback = "Heron/" + filename;
			file.open(fallback, std::ios::ate | std::ios::binary);
			if (!file.is_open()) {
				// Fallback 2: check if we are in build/HeronEditor
				std::string fallback3 = "../../Heron/" + filename;
				file.open(fallback3, std::ios::ate | std::ios::binary);
				if (!file.is_open()) {
					HRN_CORE_ERROR("Failed to open file: {0}", filename);
					return std::vector<char>();
				}
			}
		}
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();
		return buffer;
	}

	static VkShaderModule CreateShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(VulkanContext::Get()->GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to create shader module!");
		}
		return shaderModule;
	}

	VulkanShader::VulkanShader(const std::string& vertexSrcPath, const std::string& fragmentSrcPath)
	{
		HRN_CORE_INFO("Loading Vulkan Shader: {0} and {1}", vertexSrcPath, fragmentSrcPath);
		
		auto vertCode = ReadFile(vertexSrcPath);
		auto fragCode = ReadFile(fragmentSrcPath);

		m_VertexModule = CreateShaderModule(vertCode);
		m_FragmentModule = CreateShaderModule(fragCode);
	}

	VulkanShader::~VulkanShader()
	{
		VkDevice device = VulkanContext::Get()->GetDevice();
		if (m_UniformBuffer) vkDestroyBuffer(device, m_UniformBuffer, nullptr);
		if (m_UniformBufferMemory) vkFreeMemory(device, m_UniformBufferMemory, nullptr);
		if (m_Pipeline) vkDestroyPipeline(device, m_Pipeline, nullptr);
		if (m_PipelineLayout) vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
		if (m_DescriptorSetLayout) vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
		if (m_VertexModule) vkDestroyShaderModule(device, m_VertexModule, nullptr);
		if (m_FragmentModule) vkDestroyShaderModule(device, m_FragmentModule, nullptr);
	}

	static VkFormat ShaderDataTypeToVulkanFormat(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:    return VK_FORMAT_R32_SFLOAT;
			case ShaderDataType::Float2:   return VK_FORMAT_R32G32_SFLOAT;
			case ShaderDataType::Float3:   return VK_FORMAT_R32G32B32_SFLOAT;
			case ShaderDataType::Float4:   return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ShaderDataType::Mat3:     return VK_FORMAT_R32G32B32_SFLOAT; // Actually takes 3 slots
			case ShaderDataType::Mat4:     return VK_FORMAT_R32G32B32A32_SFLOAT; // Takes 4 slots
			case ShaderDataType::Int:      return VK_FORMAT_R32_SINT;
			case ShaderDataType::Int2:     return VK_FORMAT_R32G32_SINT;
			case ShaderDataType::Int3:     return VK_FORMAT_R32G32B32_SINT;
			case ShaderDataType::Int4:     return VK_FORMAT_R32G32B32A32_SINT;
			case ShaderDataType::Bool:     return VK_FORMAT_R32_SINT;
		}
		HRN_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return VK_FORMAT_UNDEFINED;
	}

	void VulkanShader::BuildPipeline(VkRenderPass renderPass, const Heron::BufferLayout& layout, bool depthTest, bool isWireframe, VkPrimitiveTopology topology)
	{
		if (m_IsPipelineBuilt) return;
		
		VkDevice device = VulkanContext::Get()->GetDevice();

		// 1. Descriptor Set Layout
		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 0;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 1;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { samplerLayoutBinding, uboLayoutBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to create descriptor set layout!");
		}

		// 2. Allocate Descriptor Set
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = VulkanContext::Get()->GetDescriptorPool();
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_DescriptorSetLayout;

		if (vkAllocateDescriptorSets(device, &allocInfo, &m_DescriptorSet) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to allocate descriptor set!");
		}

		// Create UBO and initialize descriptor
		VulkanContext::Get()->CreateBuffer(sizeof(SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffer, m_UniformBufferMemory);

		if (m_UniformBufferMemory) {
			void* mappedData;
			vkMapMemory(device, m_UniformBufferMemory, 0, sizeof(SceneData), 0, &mappedData);
			memcpy(mappedData, &m_SceneData, sizeof(SceneData));
			vkUnmapMemory(device, m_UniformBufferMemory);
		}

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_UniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(SceneData);

		VkWriteDescriptorSet descriptorWriteUBO{};
		descriptorWriteUBO.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWriteUBO.dstSet = m_DescriptorSet;
		descriptorWriteUBO.dstBinding = 1;
		descriptorWriteUBO.dstArrayElement = 0;
		descriptorWriteUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWriteUBO.descriptorCount = 1;
		descriptorWriteUBO.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(device, 1, &descriptorWriteUBO, 0, nullptr);

		// 3. Push Constants
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstantData);

		// 4. Pipeline Layout
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to create pipeline layout!");
		}

		// 5. Shader Stages
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = m_VertexModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = m_FragmentModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// 6. Vertex Input
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = layout.GetStride();
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		uint32_t location = 0;
		for (const auto& element : layout) {
			VkVertexInputAttributeDescription attrDesc{};
			attrDesc.binding = 0;
			attrDesc.location = location;
			attrDesc.format = ShaderDataTypeToVulkanFormat(element.Type);
			attrDesc.offset = element.Offset;
			attributeDescriptions.push_back(attrDesc);
			location++;
		}

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// 7. Input Assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = topology;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// 8. Viewport & Scissor (Dynamic)
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicStateInfo.pDynamicStates = dynamicStates.data();

		// 9. Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = isWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE; // Disable culling due to OpenGL vs Vulkan Y-flip
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		// 10. Multisample
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// 11. Depth Stencil
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
		depthStencil.depthWriteEnable = depthTest ? VK_TRUE : VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // Helps with skybox
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		// 12. Color Blend
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		// CREATE PIPELINE
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicStateInfo;
		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to create graphics pipeline!");
		}

		m_IsPipelineBuilt = true;
	}

	void VulkanShader::PushConstants(VkCommandBuffer cmdBuffer) const
	{
		vkCmdPushConstants(cmdBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), &m_PushConstants);
	}

	void VulkanShader::Bind() const
	{
		VkCommandBuffer cmdBuffer = VulkanContext::Get()->GetActiveCommandBuffer();
		if (cmdBuffer && m_Pipeline) {
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
			if (m_DescriptorSet) {
				// Update Descriptor Set with the active texture at slot 0 only if changed
				VkImageView imageView = VulkanContext::Get()->GetBoundImageView(0);
				VkSampler sampler = VulkanContext::Get()->GetBoundSampler(0);

				if (imageView && sampler && (imageView != m_LastImageView || sampler != m_LastSampler)) {
					VkDescriptorImageInfo imageInfo{};
					imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfo.imageView = imageView;
					imageInfo.sampler = sampler;

					VkWriteDescriptorSet descriptorWrite{};
					descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite.dstSet = m_DescriptorSet;
					descriptorWrite.dstBinding = 0;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrite.descriptorCount = 1;
					descriptorWrite.pImageInfo = &imageInfo;

					vkUpdateDescriptorSets(VulkanContext::Get()->GetDevice(), 1, &descriptorWrite, 0, nullptr);
                    
					m_LastImageView = imageView;
					m_LastSampler = sampler;
				}

				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);
			}
			PushConstants(cmdBuffer);
		}
	}

	void VulkanShader::Unbind() const
	{
	}

	void VulkanShader::SetMat4(const std::string& name, const glm::mat4& value)
	{
		if (name == "u_Model" || name == "u_View") m_PushConstants.Model = value;
		if (auto cmd = VulkanContext::Get()->GetActiveCommandBuffer()) PushConstants(cmd);
	}

	void VulkanShader::SetFloat3(const std::string& name, const glm::vec3& value)
	{
	}

	void VulkanShader::SetFloat(const std::string& name, float value)
	{
	}

	void VulkanShader::SetFloat4(const std::string& name, const glm::vec4& value)
	{
		if (name == "u_Color") m_PushConstants.Color = value;
		if (auto cmd = VulkanContext::Get()->GetActiveCommandBuffer()) PushConstants(cmd);
	}

	void VulkanShader::SetInt(const std::string& name, int value)
	{
	}

	void VulkanShader::SetSceneData(const SceneData& data)
	{
		m_SceneData = data;
		if (m_UniformBufferMemory) {
			void* mappedData;
			vkMapMemory(VulkanContext::Get()->GetDevice(), m_UniformBufferMemory, 0, sizeof(SceneData), 0, &mappedData);
			memcpy(mappedData, &data, sizeof(SceneData));
			vkUnmapMemory(VulkanContext::Get()->GetDevice(), m_UniformBufferMemory);
		}
	}

}
