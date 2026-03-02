#pragma once

#include <vector>

class VulkanApplication;
class GLFW;

class AppList
{
public:
	AppList(const std::string& groupName = "")
		: name(groupName)
	{
	}

	AppList(const AppList&) = delete;
	AppList& operator=(const AppList&) = delete;

	AppList(AppList&&) = default;
	AppList& operator=(AppList&&) = default;

	void SetName(const std::string& groupName) { name = groupName; }

	void Add(VulkanApplication& app)
	{
		apps.push_back(std::unique_ptr<VulkanApplication>(&app));
	}

	size_t GetNumApps() const { return apps.size(); }

	VulkanApplication& GetApp(size_t index) { return *apps[index].get(); }

private:
	std::vector<std::unique_ptr<VulkanApplication>> apps;
	std::string name;
};

class AppGroups
{
public:
	AppGroups();
	~AppGroups();

	VulkanApplication& GetCurrentApp()
	{
		return groups[currentGroup].GetApp(currentSubgroup);
	}

	bool MoveToNextGroup()
	{
		if (currentGroup + 1 < (int)groups.size())
		{
			groupPosition[currentGroup] = currentSubgroup;
			currentGroup++;
			currentSubgroup = groupPosition[currentGroup];
			return true;
		}
		else
			return false;
	}
	bool MoveToPreviousGroup()
	{
		if (currentGroup > 0)
		{
			groupPosition[currentGroup] = currentSubgroup;
			currentGroup--;
			currentSubgroup = groupPosition[currentGroup];
			return true;
		}
		else
			return false;
	}
	bool MoveToNextSubgroup()
	{
		if (currentSubgroup + 1 < (int)groups[currentGroup].GetNumApps())
		{
			currentSubgroup++;
			return true;
		}
		else
			return false;
	}
	bool MoveToPreviousSubgroup()
	{
		if (currentSubgroup > 0)
		{
			currentSubgroup--;
			return true;
		}
		else
			return false;
	}

	bool CheckKeys(GLFW& window);

	bool SceneChanged()
	{
		if (newScene)
		{
			newScene = false;
			return true;
		}
		else
			return false;
	}

	uint32_t GetCurrentGroupNum() { return currentGroup + 1; }
	uint32_t GetNumGroups() { return (uint32_t)groups.size(); }
	uint32_t GetCurrentSubgroupNum() { return currentSubgroup + 1; }
	uint32_t GetNumSubGroups() { return (uint32_t)groups[currentGroup].GetNumApps(); }

protected:
	void AddGroup(AppList &apps)
	{
		groups.push_back(std::move(apps));
		groupPosition.push_back(0);
	}
	// Helper functions to create apps
	struct AppDetails
	{
		VulkanApplication *app;
		std::string appName;
		int appResourceID;
	};
	void AddApp(AppList& list, VulkanApplication& app, const std::string& groupName, const std::string& appName, int resourceId);
	void AddApp(AppList& list, VulkanApplication& (*createFunc)(), const std::string& groupName, const std::string& appName, int resourceId);
	void AddAppGroup(const std::string& groupName, std::vector<AppDetails> appDetails);

private:
	std::vector<AppList> groups;
	std::vector<uint32_t> groupPosition;
	uint32_t currentGroup;
	uint32_t currentSubgroup;
	bool newScene = false;

	bool fontsEnabled;
};

extern VulkanApplication& CreatePipelinesScene1App();
extern VulkanApplication& CreatePipelinesScene2App();
extern VulkanApplication& CreatePipelinesScene3App();
extern VulkanApplication& CreatePipelinesDynamicViewportsApp();
extern VulkanApplication& CreatePipelinesMultipleApp();
extern VulkanApplication& CreatePipelinesMultipleDerivedApp();

extern VulkanApplication& CreateDescriptorSetsCube1App();
extern VulkanApplication& CreateDescriptorSetsCube2App();
extern VulkanApplication& CreateDescriptorSetsTwoCubesApp();
extern VulkanApplication& CreateDescriptorSetsLitCubesApp();

extern VulkanApplication& CreateDynamicUniformBufferMultipleCubesApp();
extern VulkanApplication& CreateDynamicUniformBufferCubeOfCubesApp();
extern VulkanApplication& CreateDynamicUniformBufferSpinningCubesApp();
extern VulkanApplication& CreateDynamicUniformBufferLitCubesApp();

extern VulkanApplication& CreatePushConstantsModelApp();
extern VulkanApplication& CreatePushConstantsUniformSpotApp();
extern VulkanApplication& CreatePushConstantsPushConstantSpotApp();
extern VulkanApplication& CreatePushConstantsMultiSpotApp();
extern VulkanApplication& CreatePushConstantsCubesApp();

extern VulkanApplication& CreateSpecializationConstantsScene1App();
extern VulkanApplication& CreateSpecializationConstantsScene2App();
extern VulkanApplication& CreateSpecializationConstantsScene3App();
extern VulkanApplication& CreateSpecializationConstantsMultipleScenesApp();
extern VulkanApplication& CreateSpecializationConstantsUberShaderApp();
extern VulkanApplication& CreateSpecializationConstantsUberShaderSingleApp();

extern VulkanApplication& CreateTextureShieldApp();
extern VulkanApplication& CreateTextureLoDApp();
extern VulkanApplication& CreateTextureLoDTestApp();

extern VulkanApplication& CreateTextureArrayTextureArrayApp();
extern VulkanApplication& CreateTextureArrayInstancingApp();
extern VulkanApplication& CreateTextureArraySamplerArrayApp();

extern VulkanApplication& CreateCubeMapTextureArrayApp();
extern VulkanApplication& CreateCubeMapUnboxedApp();
extern VulkanApplication& CreateCubeMapBoxedApp();
extern VulkanApplication& CreateCubeMapCubeMapApp();
extern VulkanApplication& CreateCubeMapModelsApp();
extern VulkanApplication& CreateCubeMapReflectModelsApp();
extern VulkanApplication& CreateCubeMapCubeMapPlusModelsApp();

extern VulkanApplication& CreateTextures3dSamplerArrayQuadsApp();
extern VulkanApplication& CreateTextures3dSamplerArrayMoreQuadsApp();
extern VulkanApplication& CreateTextures3dMoreQuads3dApp();
extern VulkanApplication& CreateTextures3dNoise3dApp();

extern VulkanApplication& CreateModelTestModelApp();
extern VulkanApplication& CreateModelTestMeshApp();
extern VulkanApplication& CreateModelLitTestMeshApp();

extern VulkanApplication& CreateInputAttachmentsSceneApp();
extern VulkanApplication& CreateInputAttachmentsDepthApp();
extern VulkanApplication& CreateInputAttachmentsMultipassApp();

extern VulkanApplication& CreateSubPassesSceneApp();
extern VulkanApplication& CreateSubPassesGBufferApp();
extern VulkanApplication& CreateSubPassesGBufferDrawApp();
extern VulkanApplication& CreateSubPassesGBufferGlassApp();
extern VulkanApplication& CreateSubPassesGBufferLitApp();

extern VulkanApplication& CreateOffscreenSceneApp();
extern VulkanApplication& CreateOffscreenOffscreenApp();
extern VulkanApplication& CreateOffscreenReflectApp();

extern VulkanApplication& CreateParticlesNormalMapApp();
extern VulkanApplication& CreateParticlesFlameApp();
extern VulkanApplication& CreateParticlesSmokeApp();
extern VulkanApplication& CreateParticlesFireApp();
extern VulkanApplication& CreateParticlesCombinedApp();

extern VulkanApplication& CreateStencilBufferSceneApp();
extern VulkanApplication& CreateStencilBufferStencilApp();
extern VulkanApplication& CreateStencilBufferCutOutApp();
extern VulkanApplication& CreateStencilBufferOutlineApp();
extern VulkanApplication& CreateStencilBufferHighlightApp();
extern VulkanApplication& CreateStencilBufferObscuredApp();
