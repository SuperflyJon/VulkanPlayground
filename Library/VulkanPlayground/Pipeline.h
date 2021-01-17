#pragma once

#include "Common.h"
#include "Shader.h"

class RenderPass;
class Descriptor;
class VulkanSystem;

struct VertexDescription
{
	VertexDescription() : bindingDescription{}, attributeDescriptions{}
	{}

	VkVertexInputBindingDescription bindingDescription;
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
};

class Pipeline : public ITidy
{
public:
	Pipeline();

	void DerivePipeline(const Pipeline& parentPipeline);

	void SetupVertexDescription(const std::vector<Attribs::Attrib>& attribs);

	void Create(VulkanSystem& system, VkDescriptorSetLayout descriptorSetLayout, const RenderPass& renderPass, const std::string& debugName);

	void Bind(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet = nullptr, uint32_t dynamicOffset = INVALID_VALUE) const;
	void Bind(VkCommandBuffer commandBuffer, const Descriptor& descriptor) const;

	void Tidy(VulkanSystem& system) override;

	Shader& LoadShader(VulkanSystem& system, const std::string& shaderName) { return LoadShaderDiffNames(system, shaderName, shaderName); }
	Shader& LoadShaderWithGeomShader(VulkanSystem& system, const std::string& shaderName, const std::string& dirHint = PROJECT_NAME) { return LoadShaderDiffNames(system, shaderName, shaderName, shaderName, dirHint); }
	Shader& LoadShaderDiffNames(VulkanSystem& system, const std::string& vertexShader, const std::string& fragmentShader = "", const std::string& geometryShader = "", const std::string& dirHint = PROJECT_NAME);

	void AddPushConstant(VulkanSystem& system, uint32_t dataSize, VkShaderStageFlagBits stage);
	void PushConstant(VkCommandBuffer commandBuffer, const void* data);

	void SetPolygonMode(VkPolygonMode value) { rasterizer.polygonMode = value; }
	void SetLineWidth(float value) { rasterizer.lineWidth = value; }
	void EnableDynamicState(VkDynamicState state) { dynamicStateEnables.push_back(state); }
	bool IsDynamicStateEnabled(VkDynamicState state) { return std::find(dynamicStateEnables.begin(), dynamicStateEnables.end(), state) != dynamicStateEnables.end(); }
	void SetViewPort(const VkRect2D& rect);
	void SetScissorRect(const VkRect2D& rect);
	void SetPipelineFlags(VkPipelineCreateFlags flags) { pipelineInfo.flags = flags; }
	void SetWindingOrder(VkFrontFace order) { rasterizer.frontFace = order; }
	void SetCullMode(VkCullModeFlagBits mode) { rasterizer.cullMode = mode; }
	void DisableRasterizer() { rasterizer.rasterizerDiscardEnable = VK_TRUE; }
	void SetDepthTestOp(VkCompareOp op) { depthStencil.depthCompareOp = op; }
	void SetDepthWriteEnabled(bool enabled) { depthStencil.depthWriteEnable = enabled ? VK_TRUE : VK_FALSE; }
	void SetDepthTestEnabled(bool enabled) { depthStencil.depthTestEnable = enabled ? VK_TRUE : VK_FALSE; }
	void SetStencilTestEnabled(bool enabled) { depthStencil.stencilTestEnable = enabled ? VK_TRUE : VK_FALSE; }
	void EnableStencilTest(VkCompareOp compareOp, VkStencilOp Op, uint32_t value = 1);
	void SetSubpass(uint32_t subpass) { pipelineInfo.subpass = subpass; }
	void EnableBlending(VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, uint32_t numAttachements = 1);

	bool Created() const { return pipeline != nullptr; }

	uint32_t GetStride() const { return vertexDescription.bindingDescription.stride; }

	void SetTopology(VkPrimitiveTopology topology) { inputAssembly.topology = topology; }

	void Recreate() { recreate = true; }
	bool CheckRecreate(VulkanSystem& system, RenderPass& renderPass);

protected:
	Pipeline& operator=(const Pipeline& other);

private:
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	Shader shader;

	VertexDescription vertexDescription;
	std::vector<VkDynamicState> dynamicStateEnables;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	VkViewport viewport{};
	VkRect2D scissor{};
	VkPushConstantRange pushConstantRange{};
	VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
	VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

	bool recreate;
	VkDescriptorSetLayout descriptorSetLayoutUsed;
};
