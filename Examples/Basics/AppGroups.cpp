#include "AppGroups.h"
#include <VulkanPlayground\Includes.h>
#include <VulkanPlayground\GLFW.h>
#include "AppGroups.h"
#include "resource.h"
#include "Basics.h"

#define ADD_APP(apps, name) \
	extern VulkanApplication& Create ## apps ## name ## App(); \
	auto& apps ## name = Create ## apps ## name ## App(); \
	if (fontsEnabled) apps ## name.SetTextHelper(new BasicStrings(*this, #apps, #name, IDR_ ## apps ## _ ## name ##)); \
	apps ## Apps.push_back(&apps ## name)

AppGroups::AppGroups(bool fontsEnabled)
{
	AppList TriangleApps;
	ADD_APP(Triangle, Simple);
	ADD_APP(Triangle, 3d);
	ADD_APP(Triangle, Light);
	AddGroup(TriangleApps);

	AppList PipelinesApps;
	ADD_APP(Pipelines, Scene1);
	ADD_APP(Pipelines, Scene2);
	ADD_APP(Pipelines, Scene3);
	ADD_APP(Pipelines, DynamicViewports);
	ADD_APP(Pipelines, Multiple);
	ADD_APP(Pipelines, MultipleDerived);
	AddGroup(PipelinesApps);

	AppList DescriptorSetsApps;
	ADD_APP(DescriptorSets, Cube1);
	ADD_APP(DescriptorSets, Cube2);
	ADD_APP(DescriptorSets, TwoCubes);
	ADD_APP(DescriptorSets, LitCubes);
	AddGroup(DescriptorSetsApps);

	AppList DynamicUniformBufferApps;
	ADD_APP(DynamicUniformBuffer, MultipleCubes);
	ADD_APP(DynamicUniformBuffer, CubeOfCubes);
	ADD_APP(DynamicUniformBuffer, SpinningCubes);
	ADD_APP(DynamicUniformBuffer, LitCubes);
	AddGroup(DynamicUniformBufferApps);

	AppList PushConstantsApps;
	ADD_APP(PushConstants, Model);
	ADD_APP(PushConstants, UniformSpot);
	ADD_APP(PushConstants, PushConstantSpot);
	ADD_APP(PushConstants, MultiSpot);
	ADD_APP(PushConstants, Cubes);
	AddGroup(PushConstantsApps);

	AppList SpecializationConstantsApps;
	ADD_APP(SpecializationConstants, Scene1);
	ADD_APP(SpecializationConstants, Scene2);
	ADD_APP(SpecializationConstants, Scene3);
	ADD_APP(SpecializationConstants, MultipleScenes);
	ADD_APP(SpecializationConstants, UberShader);
	ADD_APP(SpecializationConstants, UberShaderSingle);
	AddGroup(SpecializationConstantsApps);

	AppList TextureApps;
	ADD_APP(Texture, Shield);
	ADD_APP(Texture, LoD);
	ADD_APP(Texture, LoDTest);
	AddGroup(TextureApps);

	AppList CubeMapApps;
	ADD_APP(CubeMap, TextureArray);
	ADD_APP(CubeMap, Unboxed);
	ADD_APP(CubeMap, Boxed);
	ADD_APP(CubeMap, CubeMap);
	ADD_APP(CubeMap, Models);
	ADD_APP(CubeMap, ReflectModels);
	ADD_APP(CubeMap, CubeMapPlusModels);
	AddGroup(CubeMapApps);

	AppList TextureArrayApps;
	ADD_APP(TextureArray, TextureArray);
	ADD_APP(TextureArray, Instancing);
	ADD_APP(TextureArray, SamplerArray);
	AddGroup(TextureArrayApps);

	AppList Textures3dApps;
	ADD_APP(Textures3d, SamplerArrayQuads);
	ADD_APP(Textures3d, SamplerArrayMoreQuads);
	ADD_APP(Textures3d, MoreQuads3d);
	ADD_APP(Textures3d, Noise3d);
	AddGroup(Textures3dApps);

	AppList ModelApps;
	ADD_APP(Model, TestModel);
	ADD_APP(Model, TestMesh);
	ADD_APP(Model, LitTestMesh);
	AddGroup(ModelApps);

	AppList InputAttachmentsApps;
	ADD_APP(InputAttachments, Scene);
	ADD_APP(InputAttachments, Depth);
	ADD_APP(InputAttachments, Multipass);
	AddGroup(InputAttachmentsApps);

	AppList SubPassesApps;
	ADD_APP(SubPasses, Scene);
	ADD_APP(SubPasses, GBuffer);
	ADD_APP(SubPasses, GBufferDraw);
	ADD_APP(SubPasses, GBufferGlass);
	ADD_APP(SubPasses, GBufferLit);
	AddGroup(SubPassesApps);

	AppList OffscreenApps;
	ADD_APP(Offscreen, Scene);
	ADD_APP(Offscreen, Offscreen);
	ADD_APP(Offscreen, Reflect);
	AddGroup(OffscreenApps);

	AppList ParticlesApps;
	ADD_APP(Particles, NormalMap);
	ADD_APP(Particles, Flame);
	ADD_APP(Particles, Smoke);
	ADD_APP(Particles, Fire);
	ADD_APP(Particles, Combined);
	AddGroup(ParticlesApps);

	AppList StencilBufferApps;
	ADD_APP(StencilBuffer, Scene);
	ADD_APP(StencilBuffer, Stencil);
	ADD_APP(StencilBuffer, CutOut);
	ADD_APP(StencilBuffer, Outline);
	ADD_APP(StencilBuffer, Highlight);
	ADD_APP(StencilBuffer, Obscured);
	AddGroup(StencilBufferApps);

	// Set random background colours to distinguish similar scenes
	for (int groupNum = 0; groupNum < groups.size(); groupNum++)
	{
		srand(groupNum);
		for (int appNum = 0; appNum < groups[groupNum].size(); appNum++)
		{
			groups[groupNum][appNum]->SetClearColour({rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, 0});
		}
	}

	currentGroup = 0;
	currentSubgroup = 0;
}

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
