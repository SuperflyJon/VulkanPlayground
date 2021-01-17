#include "stdafx.h"
#include "Pipeline.h"
#include "System.h"
#include "RenderPass.h"
#include "Shader.h"
#include "Descriptor.h"

void Pipeline::SetViewPort(const VkRect2D& rect)
{
	viewport.x = (float)rect.offset.x;
	viewport.y = (float)rect.offset.y;
	viewport.width = (float)rect.extent.width;
	viewport.height = (float)rect.extent.height;
}

void Pipeline::SetScissorRect(const VkRect2D& rect)
{
	scissor = rect;
}

Shader& Pipeline::LoadShaderDiffNames(VulkanSystem& system, const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader, const std::string& dirHint)
{
	auto vertexFile = vertexShader.empty() ? "" : (dirHint + "\\" + vertexShader);
	auto fragmentFile = fragmentShader.empty() ? "" : (dirHint + "\\" + fragmentShader);
	auto geomFile = geometryShader.empty() ? "" : (dirHint + "\\" + geometryShader);
	shader.Load(system, vertexFile, fragmentFile, geomFile);
	return shader;
}

void Pipeline::AddPushConstant(VulkanSystem& system, uint32_t dataSize, VkShaderStageFlagBits stage)
{
	// Check requested push constant size against hardware limit
	// Specs require 128 bytes, so if the device complies our push constant buffer should always fit into memory		
	if (dataSize > system.GetDeviceProperties().limits.maxPushConstantsSize)
		throw std::runtime_error("Push constant too big!");

	pushConstantRange.stageFlags = stage;
	pushConstantRange.offset = 0;
	pushConstantRange.size = dataSize;
}

void Pipeline::PushConstant(VkCommandBuffer commandBuffer, const void* data)
{
	vkCmdPushConstants(commandBuffer, pipelineLayout, pushConstantRange.stageFlags, pushConstantRange.offset, pushConstantRange.size, data);
}

Pipeline::Pipeline()
	: pipelineLayout(nullptr), pipeline(nullptr), recreate(false)
{
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;

	// Disable scissoring
	scissor.extent.width = 10000;
	scissor.extent.height = 10000;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	colorBlending.logicOpEnable = VK_FALSE;

	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;	// Not used (test disabled)
	depthStencil.maxDepthBounds = 1.0f;	// Not used (test disabled)
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};
}

void Pipeline::Create(VulkanSystem& system, VkDescriptorSetLayout descriptorSetLayout, const RenderPass& renderPass, const std::string& debugName)
{
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	if (renderPass.HasDepthBuffer())
		pipelineInfo.pDepthStencilState = &depthStencil;

	colorBlending.attachmentCount = (uint32_t)renderPass.GetNumColourAttacachments(pipelineInfo.subpass);
	// disable blending unless colorBlendAttachments populated
	for (uint32_t disabled = (uint32_t)colorBlendAttachments.size(); disabled < colorBlending.attachmentCount; disabled++)
	{
		VkPipelineColorBlendAttachmentState disabledState{};
		disabledState.blendEnable = VK_FALSE;
		disabledState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachments.push_back(disabledState);
	}
	colorBlending.pAttachments = colorBlendAttachments.data();

	pipelineInfo.pColorBlendState = &colorBlending;

	// Create Graphics Pipeline
	if (vertexDescription.attributeDescriptions.size() > 0)
	{
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &vertexDescription.bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertexDescription.attributeDescriptions.size();
		vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributeDescriptions.data();
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	if (descriptorSetLayout != nullptr)
	{
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		descriptorSetLayoutUsed = descriptorSetLayout;
	}
	if (pushConstantRange.size > 0)
	{
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	}
	CHECK_VULKAN(vkCreatePipelineLayout(system.GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout), "Failed to createpipeline layout!");

	pipelineInfo.stageCount = shader.NumShaders();
	if (pipelineInfo.stageCount > 0)
		pipelineInfo.pStages = shader.StageData();

	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	if (!dynamicStateEnables.empty())
	{
		pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
		pipelineDynamicStateCreateInfo.dynamicStateCount = (uint32_t)(dynamicStateEnables.size());
		pipelineDynamicStateCreateInfo.flags = 0;
		pipelineInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
	}
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass.Get();
	CHECK_VULKAN(vkCreateGraphicsPipelines(system.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), "Failed to create graphics pipeline!");

	system.DebugNameObject(pipeline, VK_OBJECT_TYPE_PIPELINE, "Pipeline", debugName);
	auto inc = system.GetScopedDebugOutputIncrement();
	system.DebugNameObject(pipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout", debugName);
}

void Pipeline::Bind(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, uint32_t dynamicOffset) const
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	if (descriptorSet != nullptr)
	{
		if (dynamicOffset != INVALID_VALUE)
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 1, &dynamicOffset);
		else
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	}
}

void Pipeline::Bind(VkCommandBuffer commandBuffer, const Descriptor& descriptor) const
{
	Bind(commandBuffer, descriptor.GetDescriptorSet());
}

void Pipeline::SetupVertexDescription(const std::vector<Attribs::Attrib>& attribs)
{
	vertexDescription.attributeDescriptions.clear();
	uint32_t offset = 0;
	for (auto& attrib : attribs)
	{
		VkVertexInputAttributeDescription attributeDescription;
		attributeDescription.binding = 0;
		attributeDescription.location = attrib.pos;
		attributeDescription.format = attrib.format;
		attributeDescription.offset = offset;
		vertexDescription.attributeDescriptions.push_back(attributeDescription);

		offset += Attribs::FormatSize(attrib.format);
	}

	vertexDescription.bindingDescription.binding = 0;
	vertexDescription.bindingDescription.stride = offset;
	vertexDescription.bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void Pipeline::Tidy(VulkanSystem& system)
{
	if (pipeline != nullptr)
		vkDestroyPipeline(system.GetDevice(), pipeline, nullptr);
	pipeline = nullptr;

	if (pipelineLayout != nullptr)
		vkDestroyPipelineLayout(system.GetDevice(), pipelineLayout, nullptr);
	pipelineLayout = nullptr;

	shader.Tidy(system);

	dynamicStateEnables.clear();
}

void Pipeline::DerivePipeline(const Pipeline& parentPipeline)
{
	*this = parentPipeline;

	SetPipelineFlags(VK_PIPELINE_CREATE_DERIVATIVE_BIT);
	pipelineInfo.basePipelineHandle = parentPipeline.pipeline;
	pipelineInfo.basePipelineIndex = -1;
}

void Pipeline::EnableBlending(VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor, uint32_t numAttachements)
{	// Enable standard blending
	VkPipelineColorBlendAttachmentState enabledState{};
	enabledState.blendEnable = VK_TRUE;
	enabledState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	enabledState.srcColorBlendFactor = srcColorBlendFactor;
	enabledState.dstColorBlendFactor = dstColorBlendFactor;
	enabledState.colorBlendOp = VK_BLEND_OP_ADD;
	enabledState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	enabledState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	enabledState.alphaBlendOp = VK_BLEND_OP_ADD;

	for (uint32_t count = 0; count < numAttachements; count++)
	{
		colorBlendAttachments.push_back(enabledState);
	}
}

void Pipeline::EnableStencilTest(VkCompareOp compareOp, VkStencilOp Op, uint32_t value)
{
	depthStencil.stencilTestEnable = true;
	depthStencil.back.passOp = Op;
	depthStencil.back.failOp = Op;
	depthStencil.back.depthFailOp = Op;
	depthStencil.back.compareOp = compareOp;
	depthStencil.back.compareMask = 0xff;
	depthStencil.back.writeMask = 0xff;
	depthStencil.back.reference = value;
	depthStencil.front = depthStencil.back;
}

Pipeline& Pipeline::operator=(const Pipeline& other)
{
	pipelineLayout = other.pipelineLayout;

	vertexDescription = other.vertexDescription;
	dynamicStateEnables = other.dynamicStateEnables;

	vertexInputInfo = other.vertexInputInfo;
	inputAssembly = other.inputAssembly;
	viewportState = other.viewportState;
	viewport = other.viewport;
	scissor = other.scissor;
	pushConstantRange = other.pushConstantRange;
	rasterizer = other.rasterizer;
	multisampling = other.multisampling;
	colorBlendAttachments = other.colorBlendAttachments;
	colorBlending = other.colorBlending;
	depthStencil = other.depthStencil;
	pipelineInfo = other.pipelineInfo;

	pipeline = nullptr;

	return *this;
}

bool Pipeline::CheckRecreate(VulkanSystem& system, RenderPass& renderPass)
{
	if (recreate)
	{
		recreate = false;

		if (pipeline != nullptr)
		{
			system.DeviceWaitIdle();	// Ensure not in use
			vkDestroyPipeline(system.GetDevice(), pipeline, nullptr);
			pipeline = nullptr;
			vkDestroyPipelineLayout(system.GetDevice(), pipelineLayout, nullptr);
			pipelineLayout = nullptr;
			Create(system, descriptorSetLayoutUsed, renderPass, "pipelineRecreated");
			return true;
		}
	}
	return false;
}
