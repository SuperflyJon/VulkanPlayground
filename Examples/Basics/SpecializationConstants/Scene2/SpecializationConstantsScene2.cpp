#include <VulkanPlayground\Includes.h>

class SpecializationConstantsScene2App : public VulkanApplication3DSimpleLight
{
	void ResetScene() override
	{
		UseEyeLightSpace();
		CalcPositionMatrix({ 0, 0, 0 }, { -0.9, -3.0, 0 }, 0, 40, model.GetModelSize());
		SetupLighting({ 0.6f, 0, -0.4f }, { 0.25f, 0.5f, 0.9f, 0.98f }, { 0.13f, 0.4f, 0.66f, 0.75f }, 2.0f, model.GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "color_teapot_spheres.dae"), Attribs::PosNormCol);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 3, lightToonUBO, "Light", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetCullMode(VK_CULL_MODE_NONE);	// Hide issues with teapot model
		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		pipeline.LoadShader(system, "ScScene2");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Model model;
};

DECLARE_APP(SpecializationConstantsScene2)
