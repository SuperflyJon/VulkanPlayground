#include <VulkanPlayground\Includes.h>

class SubPassesSceneApp : public VulkanApplication3DLight
{
	void ResetScene() override
	{
		CalcPositionMatrix({ 0.0f, 15.0f, 0.0f }, { -0.3, -0.1, -0.05 }, 0, 0, model.GetModelSize());
		SetupLighting({ 1.0f, -0.8f, -0.8f }, 0.5f, 0.7f, 0.3f, 32.0f, model.GetModelSize());
		drawGlass = true;
		drawBuilding = true;
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		// Main building
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "samplebuilding.dae"), Attribs::PosNormCol);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 2, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Main");

		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		pipeline.LoadShader(system, "SpScene");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Main");

		// Glass windows
		modelTrans.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "samplebuilding_glass.dae"), Attribs::PosTex);

		glassDescriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		glassDescriptor.AddTexture(system, 2, texture, VulkanPlayground::GetModelFile("Basics", "coloured_glass_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		CreateDescriptor(system, glassDescriptor,  "Glass");

		glassGraphicsPipeline.SetupVertexDescription(Attribs::PosTex);
		glassGraphicsPipeline.SetCullMode(VK_CULL_MODE_NONE);	// Glass is facing different ways
		glassGraphicsPipeline.SetDepthWriteEnabled(false);	// Don't overwrite windows with other windows
		glassGraphicsPipeline.EnableBlending();	// Transparent
		glassGraphicsPipeline.LoadShader(system, "glassScene");
		CreatePipeline(system, renderPass, glassGraphicsPipeline, glassDescriptor, workingExtent, "Glass");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		if (drawBuilding)
			model.Draw(commandBuffer);

		glassGraphicsPipeline.Bind(commandBuffer, glassDescriptor.GetDescriptorSet());
		if (drawGlass)
			modelTrans.Draw(commandBuffer);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('G'))
		{
			drawGlass = !drawGlass;
			RedrawScene();
		}
		if (eventData.KeyPressed('B'))
		{
			drawBuilding = !drawBuilding;
			RedrawScene();
		}
	}

private:
	Model model;
	Model modelTrans;
	Pipeline pipeline, glassGraphicsPipeline;
	Descriptor descriptor, glassDescriptor;
	Texture texture;

	bool drawGlass;
	bool drawBuilding;
};

DECLARE_APP(SubPassesScene)
