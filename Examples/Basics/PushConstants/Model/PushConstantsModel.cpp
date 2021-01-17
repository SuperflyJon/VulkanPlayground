#include <VulkanPlayground\Includes.h>

class PushConstantsModelApp : public VulkanApplication3DLight
{
	void ResetScene() override
	{
		CalcPositionMatrix({ 0, 225, 0 }, { -0.8, -0.8, 0 }, 0, 20, model.GetModelSize());
		SetupLighting({ -0.5f, -1, -0.3f }, 0.5f, 0.8f, 0.2f, 30.0f, model.GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "SampleScene.dae"), Attribs::PosNormCol);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 2, lightUBO, "Lights", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		pipeline.LoadShader(system, "SampleModel");
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

DECLARE_APP(PushConstantsModel)
