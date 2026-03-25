#include "AppGroups.h"
#include <VulkanPlayground\Includes.h>
#include <VulkanPlayground\GLFW.h>
#include "AppGroups.h"
#include "resource.h"
#include "Basics.h"

#include "Triangle/Simple/TriangleSimple.h"
#include "Triangle/Light/TriangleLight.h"
#include "Triangle/3d/Triangle3d.h"

// Helper implementation
void AppGroups::AddApp(AppList& list, VulkanApplication& app, int resourceId)
{
	app.SetTextHelper(new BasicStrings(resourceId));

	list.Add(app);
}
void AppGroups::AddApp(AppList& list, VulkanApplication& (*createFunc)(), const std::string& /*groupName*/, const std::string& /*appName*/, int resourceId)
{
	AddApp(list, createFunc(), resourceId);
}
void AppGroups::AddAppGroup(std::vector<AppDetails> appDetails)
{
	AppList newGroup;
	for (auto& appDetail : appDetails)
	{
		AddApp(newGroup, *appDetail.app, appDetail.appResourceID);
	}
	AddGroup(newGroup);
}

AppGroups::AppGroups()
{
	AddAppGroup( {
		{new TriangleSimpleApp, IDR_Triangle_Simple},
		{new Triangle3dApp, IDR_Triangle_3d},
		{new TriangleLightApp, IDR_Triangle_Light}
	});

	AppList PipelinesApps;
	AddApp(PipelinesApps, CreatePipelinesScene1App, "Pipelines", "Scene1", IDR_Pipelines_Scene1);
	AddApp(PipelinesApps, CreatePipelinesScene2App, "Pipelines", "Scene2", IDR_Pipelines_Scene2);
	AddApp(PipelinesApps, CreatePipelinesScene3App, "Pipelines", "Scene3", IDR_Pipelines_Scene3);
	AddApp(PipelinesApps, CreatePipelinesDynamicViewportsApp, "Pipelines", "DynamicViewports", IDR_Pipelines_DynamicViewports);
	AddApp(PipelinesApps, CreatePipelinesMultipleApp, "Pipelines", "Multiple", IDR_Pipelines_Multiple);
	AddApp(PipelinesApps, CreatePipelinesMultipleDerivedApp, "Pipelines", "MultipleDerived", IDR_Pipelines_MultipleDerived);
	AddGroup(PipelinesApps);

	AppList DescriptorSetsApps;
	AddApp(DescriptorSetsApps, CreateDescriptorSetsCube1App, "DescriptorSets", "Cube1", IDR_DescriptorSets_Cube1);
	AddApp(DescriptorSetsApps, CreateDescriptorSetsCube2App, "DescriptorSets", "Cube2", IDR_DescriptorSets_Cube2);
	AddApp(DescriptorSetsApps, CreateDescriptorSetsTwoCubesApp, "DescriptorSets", "TwoCubes", IDR_DescriptorSets_TwoCubes);
	AddApp(DescriptorSetsApps, CreateDescriptorSetsLitCubesApp, "DescriptorSets", "LitCubes", IDR_DescriptorSets_LitCubes);
	AddGroup(DescriptorSetsApps);

	AppList DynamicUniformBufferApps;
	AddApp(DynamicUniformBufferApps, CreateDynamicUniformBufferMultipleCubesApp, "DynamicUniformBuffer", "MultipleCubes", IDR_DynamicUniformBuffer_MultipleCubes);
	AddApp(DynamicUniformBufferApps, CreateDynamicUniformBufferCubeOfCubesApp, "DynamicUniformBuffer", "CubeOfCubes", IDR_DynamicUniformBuffer_CubeOfCubes);
	AddApp(DynamicUniformBufferApps, CreateDynamicUniformBufferSpinningCubesApp, "DynamicUniformBuffer", "SpinningCubes", IDR_DynamicUniformBuffer_SpinningCubes);
	AddApp(DynamicUniformBufferApps, CreateDynamicUniformBufferLitCubesApp, "DynamicUniformBuffer", "LitCubes", IDR_DynamicUniformBuffer_LitCubes);
	AddGroup(DynamicUniformBufferApps);

	AppList PushConstantsApps;
	AddApp(PushConstantsApps, CreatePushConstantsModelApp, "PushConstants", "Model", IDR_PushConstants_Model);
	AddApp(PushConstantsApps, CreatePushConstantsUniformSpotApp, "PushConstants", "UniformSpot", IDR_PushConstants_UniformSpot);
	AddApp(PushConstantsApps, CreatePushConstantsPushConstantSpotApp, "PushConstants", "PushConstantSpot", IDR_PushConstants_PushConstantSpot);
	AddApp(PushConstantsApps, CreatePushConstantsMultiSpotApp, "PushConstants", "MultiSpot", IDR_PushConstants_MultiSpot);
	AddApp(PushConstantsApps, CreatePushConstantsCubesApp, "PushConstants", "Cubes", IDR_PushConstants_Cubes);
	AddGroup(PushConstantsApps);

	AppList SpecializationConstantsApps;
	AddApp(SpecializationConstantsApps, CreateSpecializationConstantsScene1App, "SpecializationConstants", "Scene1", IDR_SpecializationConstants_Scene1);
	AddApp(SpecializationConstantsApps, CreateSpecializationConstantsScene2App, "SpecializationConstants", "Scene2", IDR_SpecializationConstants_Scene2);
	AddApp(SpecializationConstantsApps, CreateSpecializationConstantsScene3App, "SpecializationConstants", "Scene3", IDR_SpecializationConstants_Scene3);
	AddApp(SpecializationConstantsApps, CreateSpecializationConstantsMultipleScenesApp, "SpecializationConstants", "MultipleScenes", IDR_SpecializationConstants_MultipleScenes);
	AddApp(SpecializationConstantsApps, CreateSpecializationConstantsUberShaderApp, "SpecializationConstants", "UberShader", IDR_SpecializationConstants_UberShader);
	AddApp(SpecializationConstantsApps, CreateSpecializationConstantsUberShaderSingleApp, "SpecializationConstants", "UberShaderSingle", IDR_SpecializationConstants_UberShaderSingle);
	AddGroup(SpecializationConstantsApps);

	AppList TextureApps;
	AddApp(TextureApps, CreateTextureShieldApp, "Texture", "Shield", IDR_Texture_Shield);
	AddApp(TextureApps, CreateTextureLoDApp, "Texture", "LoD", IDR_Texture_LoD);
	AddApp(TextureApps, CreateTextureLoDTestApp, "Texture", "LoDTest", IDR_Texture_LoDTest);
	AddGroup(TextureApps);

	AppList TextureArrayApps;
	AddApp(TextureArrayApps, CreateTextureArrayTextureArrayApp, "TextureArray", "TextureArray", IDR_TextureArray_TextureArray);
	AddApp(TextureArrayApps, CreateTextureArrayInstancingApp, "TextureArray", "Instancing", IDR_TextureArray_Instancing);
	AddApp(TextureArrayApps, CreateTextureArraySamplerArrayApp, "TextureArray", "SamplerArray", IDR_TextureArray_SamplerArray);
	AddGroup(TextureArrayApps);

	AppList CubeMapApps;
	AddApp(CubeMapApps, CreateCubeMapTextureArrayApp, "CubeMap", "TextureArray", IDR_CubeMap_TextureArray);
	AddApp(CubeMapApps, CreateCubeMapUnboxedApp, "CubeMap", "Unboxed", IDR_CubeMap_Unboxed);
	AddApp(CubeMapApps, CreateCubeMapBoxedApp, "CubeMap", "Boxed", IDR_CubeMap_Boxed);
	AddApp(CubeMapApps, CreateCubeMapCubeMapApp, "CubeMap", "CubeMap", IDR_CubeMap_CubeMap);
	AddApp(CubeMapApps, CreateCubeMapModelsApp, "CubeMap", "Models", IDR_CubeMap_Models);
	AddApp(CubeMapApps, CreateCubeMapReflectModelsApp, "CubeMap", "ReflectModels", IDR_CubeMap_ReflectModels);
	AddApp(CubeMapApps, CreateCubeMapCubeMapPlusModelsApp, "CubeMap", "CubeMapPlusModels", IDR_CubeMap_CubeMapPlusModels);
	AddGroup(CubeMapApps);

	AppList Textures3dApps;
	AddApp(Textures3dApps, CreateTextures3dSamplerArrayQuadsApp, "Textures3d", "SamplerArrayQuads", IDR_Textures3d_SamplerArrayQuads);
	AddApp(Textures3dApps, CreateTextures3dSamplerArrayMoreQuadsApp, "Textures3d", "SamplerArrayMoreQuads", IDR_Textures3d_SamplerArrayMoreQuads);
	AddApp(Textures3dApps, CreateTextures3dMoreQuads3dApp, "Textures3d", "MoreQuads3d", IDR_Textures3d_MoreQuads3d);
	AddApp(Textures3dApps, CreateTextures3dNoise3dApp, "Textures3d", "Noise3d", IDR_Textures3d_Noise3d);
	AddGroup(Textures3dApps);

	AppList ModelApps;
	AddApp(ModelApps, CreateModelTestModelApp, "Model", "TestModel", IDR_Model_TestModel);
	AddApp(ModelApps, CreateModelTestMeshApp, "Model", "TestMesh", IDR_Model_TestMesh);
	AddApp(ModelApps, CreateModelLitTestMeshApp, "Model", "LitTestMesh", IDR_Model_LitTestMesh);
	AddGroup(ModelApps);

	AppList InputAttachmentsApps;
	AddApp(InputAttachmentsApps, CreateInputAttachmentsSceneApp, "InputAttachments", "Scene", IDR_InputAttachments_Scene);
	AddApp(InputAttachmentsApps, CreateInputAttachmentsDepthApp, "InputAttachments", "Depth", IDR_InputAttachments_Depth);
	AddApp(InputAttachmentsApps, CreateInputAttachmentsMultipassApp, "InputAttachments", "Multipass", IDR_InputAttachments_Multipass);
	AddGroup(InputAttachmentsApps);

	AppList SubPassesApps;
	AddApp(SubPassesApps, CreateSubPassesSceneApp, "SubPasses", "Scene", IDR_SubPasses_Scene);
	AddApp(SubPassesApps, CreateSubPassesGBufferApp, "SubPasses", "GBuffer", IDR_SubPasses_GBuffer);
	AddApp(SubPassesApps, CreateSubPassesGBufferDrawApp, "SubPasses", "GBufferDraw", IDR_SubPasses_GBufferDraw);
	AddApp(SubPassesApps, CreateSubPassesGBufferGlassApp, "SubPasses", "GBufferGlass", IDR_SubPasses_GBufferGlass);
	AddApp(SubPassesApps, CreateSubPassesGBufferLitApp, "SubPasses", "GBufferLit", IDR_SubPasses_GBufferLit);
	AddGroup(SubPassesApps);

	AppList OffscreenApps;
	AddApp(OffscreenApps, CreateOffscreenSceneApp, "Offscreen", "Scene", IDR_Offscreen_Scene);
	AddApp(OffscreenApps, CreateOffscreenOffscreenApp, "Offscreen", "Offscreen", IDR_Offscreen_Offscreen);
	AddApp(OffscreenApps, CreateOffscreenReflectApp, "Offscreen", "Reflect", IDR_Offscreen_Reflect);
	AddGroup(OffscreenApps);

	AppList ParticlesApps;
	AddApp(ParticlesApps, CreateParticlesNormalMapApp, "Particles", "NormalMap", IDR_Particles_NormalMap);
	AddApp(ParticlesApps, CreateParticlesFlameApp, "Particles", "Flame", IDR_Particles_Flame);
	AddApp(ParticlesApps, CreateParticlesSmokeApp, "Particles", "Smoke", IDR_Particles_Smoke);
	AddApp(ParticlesApps, CreateParticlesFireApp, "Particles", "Fire", IDR_Particles_Fire);
	AddApp(ParticlesApps, CreateParticlesCombinedApp, "Particles", "Combined", IDR_Particles_Combined);
	AddGroup(ParticlesApps);

	AppList StencilBufferApps;
	AddApp(StencilBufferApps, CreateStencilBufferSceneApp, "StencilBuffer", "Scene", IDR_StencilBuffer_Scene);
	AddApp(StencilBufferApps, CreateStencilBufferStencilApp, "StencilBuffer", "Stencil", IDR_StencilBuffer_Stencil);
	AddApp(StencilBufferApps, CreateStencilBufferCutOutApp, "StencilBuffer", "CutOut", IDR_StencilBuffer_CutOut);
	AddApp(StencilBufferApps, CreateStencilBufferOutlineApp, "StencilBuffer", "Outline", IDR_StencilBuffer_Outline);
	AddApp(StencilBufferApps, CreateStencilBufferHighlightApp, "StencilBuffer", "Highlight", IDR_StencilBuffer_Highlight);
	AddApp(StencilBufferApps, CreateStencilBufferObscuredApp, "StencilBuffer", "Obscured", IDR_StencilBuffer_Obscured);
	AddGroup(StencilBufferApps);

	// Setup text strings and set random background colours to distinguish similar scenes
	for (int groupNum = 0; groupNum < groups.size(); groupNum++)
	{
		srand(groupNum);
		auto numSubGroups = groups[groupNum].GetNumApps();
		for (int appNum = 0; appNum < numSubGroups; appNum++)
		{
			auto& app = groups[groupNum].GetApp(appNum);

			std::stringstream appNumStr;
			appNumStr << '(' << groupNum + 1 << '/' << groups.size() << ") ";
			std::stringstream appSubNumStr;
			appSubNumStr << " (" << appNum + 1 << '/' << numSubGroups << ")";

			auto strings = (BasicStrings*)app.GetTextHelper();
			strings->SetGroupStrings(appNumStr.str(), appSubNumStr.str());

			app.SetClearColour({ rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, 0 });
		}
	}

	currentGroup = 0;
	currentSubgroup = 0;
}

AppGroups::~AppGroups() = default;

bool AppGroups::CheckKeys(GLFW& window)
{
	// Check for arrow keys -> navigation
	if (window.KeyPressed(GLFW_KEY_DOWN))
	{
		if (MoveToNextGroup())
		{
			newScene = true;
			return true;
		}
	}
	if (window.KeyPressed(GLFW_KEY_UP))
	{
		if (MoveToPreviousGroup())
		{
			newScene = true;
			return true;
		}
	}
	if (window.KeyPressed(GLFW_KEY_RIGHT))
	{
		if (MoveToNextSubgroup())
		{
			newScene = true;
			return true;
		}
	}
	if (window.KeyPressed(GLFW_KEY_LEFT))
	{
		if (MoveToPreviousSubgroup())
		{
			newScene = true;
			return true;
		}
	}
	if (window.KeyPressed(GLFW_KEY_ENTER) || window.KeyPressed(GLFW_KEY_KP_ENTER))
	{
		if (!MoveToNextSubgroup())
		{
			if (!MoveToNextGroup())
				currentGroup = 0;	// At the end - loop back to first example

			currentSubgroup = 0;
		}
		newScene = true;
		return true;
	}

	return false;
}
