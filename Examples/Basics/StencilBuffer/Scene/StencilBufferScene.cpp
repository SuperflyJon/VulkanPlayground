#include <VulkanPlayground\Includes.h>

class StencilBufferSceneApp : public VulkanApplication3DSimpleLight
{
	void ResetScene() override
	{
		CalcPositionMatrix({ 0, 180.0f, 0.0f }, { -3.8f, -0.8f, 0 }, 0, 10, model.GetModelSize());
		SetupLighting({ 0.0f, 1.0f, 2.0f }, { 0.1f, 0.25f, 0.5f, 0.9f }, { 0.1f, 0.2f, 0.4f, 0.6f }, 1.3f, model.GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "venus.fbx"), Attribs::PosNorm);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 2, lightToonUBO, "Toon Light", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Draw");

		pipeline.SetupVertexDescription(Attribs::PosNorm);
		pipeline.LoadShader(system, "SceneN");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Main");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);
	}

private:
	Descriptor descriptor;
	Pipeline pipeline;
	Model model;
};

DECLARE_APP(StencilBufferScene)
