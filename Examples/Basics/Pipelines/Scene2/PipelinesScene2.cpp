#include <VulkanPlayground\Includes.h>

class PipelinesScene2App : public VulkanApplication3DSimpleLight
{
	void ResetScene() override
	{
		CalcPositionMatrix({ 0, -180, 0 }, { -1, -1.5, 0 }, 0, 30, model.GetModelSize());
		SetupLighting({ -1.3, 7.3, 0.7 }, { 0.1f, 0.25f, 0.35f, 0.5f }, { 0.25f, 0.5f, 0.6f, 0.75f }, 8.0f, model.GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "treasure_smooth.dae"), Attribs::PosNormCol);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 2, lightToonUBO, "Toon Light", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		pipeline.LoadShader(system, "PipelinesScene2");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "PipelinesScene2");
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

DECLARE_APP(PipelinesScene2)
