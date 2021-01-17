#include <VulkanPlayground\Includes.h>

class PipelinesScene1App : public VulkanApplication3DLight
{
	void ResetScene() override
	{
		CalcPositionMatrix({ 0, -180, 0 }, { -1, -1.5, 0 }, 0, 30, model.GetModelSize());
		SetupLighting({ -1.3, 7.3, 0.7 }, 1.0f, 1.5f, 0.1f, 70.0f, model.GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "treasure_smooth.dae"), Attribs::PosNormCol);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 1, lightUBO, "Light Info", VK_SHADER_STAGE_FRAGMENT_BIT);
		
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		pipeline.LoadShader(system, "PipelinesScene1");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "PipelinesScene1");
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

DECLARE_APP(PipelinesScene1)
