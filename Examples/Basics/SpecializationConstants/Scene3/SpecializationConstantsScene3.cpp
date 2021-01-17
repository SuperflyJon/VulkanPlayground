#include <VulkanPlayground\Includes.h>

class SpecializationConstantsScene3App : public VulkanApplication3DLight
{
	void ResetScene() override
	{
		UseEyeLightSpace();
		CalcPositionMatrix({ 0, 0, 0 }, { -0.9, -3.0, 0 }, 0, 40, model.GetModelSize());
		SetupLighting({ 0.6f, 0, -0.4f }, 0.5f, 1.0f, 0.4f, 30.0f, model.GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "color_teapot_spheres.dae"), Attribs::PosNormColTex);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 1, lightUBO, "Light", VK_SHADER_STAGE_FRAGMENT_BIT);
		descriptor.AddTexture(system, 2, texture, VulkanPlayground::GetModelFile("Basics", "metalplate_nomips_rgba.ktx"));
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetCullMode(VK_CULL_MODE_NONE);	// Hide issues with teapot model
		pipeline.SetupVertexDescription(Attribs::PosNormColTex);
		pipeline.LoadShader(system, "ScScene3");
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
	Texture texture;
	Model model;
};

DECLARE_APP(SpecializationConstantsScene3)
