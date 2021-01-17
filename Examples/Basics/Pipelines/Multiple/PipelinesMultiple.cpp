#include <VulkanPlayground\Includes.h>

class PipelinesMultipleApp : public VulkanApplication3DLight
{
	void GetRequiredDeviceFeatures(const VkPhysicalDeviceFeatures& deviceFeatures, VkPhysicalDeviceFeatures* requiredFeatures) override
	{
		VulkanPlayground::EnableFillModeNonSolid(deviceFeatures, requiredFeatures);
		VulkanPlayground::EnableWideLines(deviceFeatures, requiredFeatures);
	}

	void ResetScene() override
	{
		CalcPositionMatrix({ 0, -180, 0 }, { -1, -1.5, 0 }, 0, 30, model.GetModelSize());
		SetupLighting({ -1.3, 7.3, 0.7 }, 1.0f, 1.5f, 0.1f, 70.0f, model.GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		VulkanPlayground::SetupProjectionMatrix(mvpUBO().projection, AspectRatio() / 3);

		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "treasure_smooth.dae"), Attribs::PosNormCol);
		
		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 1, lightUBO, "Light Info", VK_SHADER_STAGE_FRAGMENT_BIT);	// For scene 1
		descriptor.AddUniformBuffer(system, 2, lightToonUBO, "Toon Light", VK_SHADER_STAGE_FRAGMENT_BIT);	// For scene 2
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline1.EnableDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
		pipeline1.LoadShader(system, "PipelinesScene1");
		pipeline1.SetupVertexDescription(Attribs::PosNormCol);
		CreatePipeline(system, renderPass, pipeline1, descriptor, workingExtent, "PipelinesScene1");

		pipeline2.EnableDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
		pipeline2.SetupVertexDescription(Attribs::PosNormCol);
		pipeline2.LoadShader(system, "PipelinesScene2");
		CreatePipeline(system, renderPass, pipeline2, descriptor, workingExtent, "PipelinesScene2");

		pipeline3.EnableDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
		pipeline3.EnableDynamicState(VK_DYNAMIC_STATE_LINE_WIDTH);
		pipeline3.SetupVertexDescription(Attribs::PosNormCol);
		pipeline3.LoadShader(system, "PipelinesScene3");
		pipeline3.SetPolygonMode(VK_POLYGON_MODE_LINE);
		CreatePipeline(system, renderPass, pipeline3, descriptor, workingExtent, "PipelinesScene3");

		VulkanApplication3DSimpleLight::SetupLighting(lightToonUBO, { -1.3, 7.3, 0.7 }, { 0.1f, 0.25f, 0.35f, 0.5f }, { 0.25f, 0.5f, 0.6f, 0.75f }, 8.0f, model.GetModelSize());
		lightToonUBO.CopyToDevice(system);
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		float widthThird = (float)GetWindowWidth() / 3.0f;
		float height = (float)GetWindowHeight();

		// Left side - First pipeline
		VulkanPlayground::SetViewport(commandBuffer, 0, 0, widthThird, height);
		pipeline1.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);

		// Middle - Second pipeline
		VulkanPlayground::SetViewport(commandBuffer, widthThird, 0, widthThird, height);
		pipeline2.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer, false);

		// Right side - Third pipeline
		VulkanPlayground::SetViewport(commandBuffer, 2 * widthThird, 0, widthThird, height);
		pipeline3.Bind(commandBuffer, descriptor);
		vkCmdSetLineWidth(commandBuffer, 2.0f);
		model.Draw(commandBuffer, false);
	}

private:
	Pipeline pipeline1, pipeline2, pipeline3;
	Descriptor descriptor;
	Model model;
	UBO<UBO_lightToon> lightToonUBO;	// For scene 2
};

DECLARE_APP(PipelinesMultiple)
