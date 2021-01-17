#include "stdafx.h"
#include "Shader.h"
#include "Common.h"
#include "System.h"
#include "WinUtil.h"

void Shader::Load(VulkanSystem& system, const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader)
{
	vertexModule = nullptr, fragmentModule = nullptr, geometryModule = nullptr;
	bool existing = true;
	if (!vertexShader.empty())
		existing &= system.FindModule(WinUtils::FindFile(vertexShader + ".vert.spv"), vertexModule);
	if (!fragmentShader.empty())
		existing &= system.FindModule(WinUtils::FindFile(fragmentShader + ".frag.spv"), fragmentModule);
	if (!geometryShader.empty())
		existing &= system.FindModule(WinUtils::FindFile(geometryShader + ".geom.spv"), geometryModule);

	if (vertexModule)
	{
		VkPipelineShaderStageCreateInfo stageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		stageInfo.module = vertexModule;
		stageInfo.pName = "main";
		shaderStages.push_back(stageInfo);
	}
	if (fragmentModule)
	{
		VkPipelineShaderStageCreateInfo stageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stageInfo.module = fragmentModule;
		stageInfo.pName = "main";
		shaderStages.push_back(stageInfo);
	}
	if (geometryModule)
	{
		VkPipelineShaderStageCreateInfo stageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		stageInfo.module = geometryModule;
		stageInfo.pName = "main";
		shaderStages.push_back(stageInfo);
	}
}

void Shader::Tidy(VulkanSystem& /*system*/) 
{
	specializationObject[0].Tidy();
	specializationObject[1].Tidy();
	shaderStages.clear();
}

SpecializationObject *Shader::GetSpecializationObject(VkShaderStageFlagBits shader)
{
	if (shader == VK_SHADER_STAGE_VERTEX_BIT)
		return &specializationObject[0];
	else
		return &specializationObject[1];
}

VkSpecializationInfo* SpecializationObject::SetSpecializationConstantsFromStruct(const void* data, const std::vector<uint32_t>& sizes)
{
	uint32_t curOffset = 0;
	for (auto constantSize : sizes)
	{
		VkSpecializationMapEntry mapEntry;
		mapEntry.constantID = (uint32_t)specializationMapEntries.size();
		mapEntry.offset = curOffset;
		mapEntry.size = constantSize;
		specializationMapEntries.push_back(mapEntry);
		curOffset += constantSize;
	}
	// Prepare specialization info block for the shader stage
	specializationInfo.dataSize = curOffset;
	specializationInfo.mapEntryCount = (uint32_t)specializationMapEntries.size();
	specializationInfo.pMapEntries = specializationMapEntries.data();
	specializationInfo.pData = data;
	return &specializationInfo;
}

VkSpecializationInfo* SpecializationObject::AddSpecializationConstant(uint32_t binding, const void* data, uint32_t size)
{
	uint32_t oldSize = (uint32_t)specializationData.size();

	VkSpecializationMapEntry mapEntry;
	mapEntry.constantID = binding;
	mapEntry.offset = oldSize;
	mapEntry.size = size;
	specializationMapEntries.push_back(mapEntry);

	uint32_t dataSize = oldSize + size;
	specializationData.resize(dataSize);
	memcpy(specializationData.data() + oldSize, data, size);

	// Prepare specialization info block for the shader stage
	specializationInfo.mapEntryCount = (uint32_t)specializationMapEntries.size();
	specializationInfo.pMapEntries = specializationMapEntries.data();
	specializationInfo.dataSize = specializationData.size();
	specializationInfo.pData = specializationData.data();
	return &specializationInfo;
}

VkPipelineShaderStageCreateInfo& Shader::FindShader(VkShaderStageFlagBits shader)
{
	auto pos = std::find_if(shaderStages.begin(), shaderStages.end(), [shader](const auto& stage) { return (stage.stage == shader); });
	if (pos == shaderStages.end())
		throw std::runtime_error("Shader stage not found!");

	return *pos;
}
