#include <VulkanPlayground\Includes.h>

class StencilBufferHighlightApp : public VulkanApplication3DSimpleLight
{
	bool UseStencilBuffer() override { return useStencil; }

	void ResetScene() override
	{
		CalcPositionMatrix({ 0, 180.0f, 0.0f }, { -3.8f, -0.8f, 0 }, 0, 10, model.GetModelSize());
		SetupLighting({ 0.0f, 1.0f, 2.0f }, { 0.1f, 0.25f, 0.5f, 0.9f }, { 0.1f, 0.2f, 0.4f, 0.6f }, 1.3f, model.GetModelSize());
		useStencil = true;
		showHighlight = true;
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "venus.fbx"), Attribs::PosNorm);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 2, lightToonUBO, "Toon Light", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Draw");

		pipeline.EnableStencilTest(VK_COMPARE_OP_ALWAYS, VK_STENCIL_OP_REPLACE);
		pipeline.SetupVertexDescription(Attribs::PosNorm);
		pipeline.LoadShader(system, "SceneN");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Model");

		highlightPipeline.EnableStencilTest(VK_COMPARE_OP_NOT_EQUAL, VK_STENCIL_OP_KEEP);
		highlightPipeline.SetDepthTestEnabled(false);	// Show highlight on top of everything
		highlightPipeline.SetupVertexDescription(Attribs::PosNorm);
		highlightPipeline.LoadShader(system, "Outline");
		CreatePipeline(system, renderPass, highlightPipeline, descriptor, workingExtent, "Highlight");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);

		if (showHighlight)
		{
			highlightPipeline.Bind(commandBuffer, descriptor);
			model.Draw(commandBuffer);
		}
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			useStencil = !useStencil;
			RecreateObjects();
		}
		if (eventData.KeyPressed('H'))
		{
			showHighlight = !showHighlight;
			RedrawScene();
		}
	}

private:
	Descriptor descriptor;
	Pipeline pipeline, highlightPipeline;
	Model model;

	bool showHighlight = true;
	bool useStencil = true;
};

DECLARE_APP(StencilBufferHighlight)
