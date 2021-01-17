#pragma once

#include "Common.h"

class SpecializationObject
{
public:
	SpecializationObject() : specializationInfo{}
	{}
	void Tidy()
	{
		specializationMapEntries.clear();
		specializationData.clear();
	}

	VkSpecializationInfo *SetSpecializationConstantsFromStruct(const void* data, const std::vector<uint32_t>& sizes);
	VkSpecializationInfo* AddSpecializationConstant(uint32_t binding, const void* data, uint32_t size);

private:
	std::vector<VkSpecializationMapEntry> specializationMapEntries;
	VkSpecializationInfo specializationInfo;
	std::vector<char>specializationData;
};

class Shader : public ITidy
{
public:
	void Load(VulkanSystem& system, const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader);
	void Tidy(VulkanSystem& system) override;

	bool Matches(VkShaderModule vertex, VkShaderModule fragment, VkShaderModule geometry) const
	{
		return (vertexModule == vertex && fragmentModule == fragment && geometryModule == geometry);
	}
	uint32_t NumShaders() const { return (uint32_t)shaderStages.size(); }
	const VkPipelineShaderStageCreateInfo* StageData() const { return shaderStages.data(); }
	VkPipelineShaderStageCreateInfo& FindShader(VkShaderStageFlagBits shader);

	void SetSpecializationConstantsFromStruct(VkShaderStageFlagBits stage, const void* data, const std::vector<uint32_t>& sizes)
	{
		FindShader(stage).pSpecializationInfo = GetSpecializationObject(stage)->SetSpecializationConstantsFromStruct(data, sizes);
	}
	template <class T> void SetSpecializationConstant(VkShaderStageFlagBits stage, uint32_t binding, const T& data)
	{
		FindShader(stage).pSpecializationInfo = GetSpecializationObject(stage)->AddSpecializationConstant(binding, &data, sizeof(data));
	}

private:
	VkShaderModule vertexModule, fragmentModule, geometryModule;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	SpecializationObject specializationObject[2];	// One for vertex and one for fragment shaders
	SpecializationObject* GetSpecializationObject(VkShaderStageFlagBits shader);
};
