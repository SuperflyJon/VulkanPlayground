
#include <VulkanPlayground\Includes.h>

class PipelinesMultipleDerivedApp : public VulkanApplication3DLight
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

		auto widthThird = workingExtent.width / 3;

		VkRect2D extent{};
		extent.extent = { widthThird, workingExtent.height };

		pipeline1.SetPipelineFlags(VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT);
		pipeline1.SetupVertexDescription(Attribs::PosNormCol);
		pipeline1.LoadShader(system, "PipelinesScene1");
		CreatePipeline(system, renderPass, pipeline1, descriptor, extent.extent, "PipelinesScene1");

		extent.offset.x += widthThird;
		pipeline1.SetViewPort(extent);	// Change parent extent as copied in next line
		pipeline2.DerivePipeline(pipeline1);

		pipeline2.LoadShader(system, "PipelinesScene2");
		CreatePipeline(system, renderPass, pipeline2, descriptor, extent.extent, "PipelinesScene2");

		// Change parent extent as copied
		extent.offset.x += widthThird;
		pipeline1.SetViewPort(extent);	// Change parent extent as copied in next line
		pipeline3.DerivePipeline(pipeline1);

		pipeline3.LoadShader(system, "PipelinesScene3");
		pipeline3.SetPolygonMode(VK_POLYGON_MODE_LINE);
		pipeline3.SetLineWidth(2.0f);
		CreatePipeline(system, renderPass, pipeline3, descriptor, extent.extent, "PipelinesScene3");

		VulkanApplication3DSimpleLight::SetupLighting(lightToonUBO, { -1.3, 7.3, 0.7 }, { 0.1f, 0.25f, 0.35f, 0.5f }, { 0.25f, 0.5f, 0.6f, 0.75f }, 8.0f, model.GetModelSize());
		lightToonUBO.CopyToDevice(system);
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		// Left side - First pipeline
		pipeline1.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);

		// Middle - Second pipeline
		pipeline2.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer, false);

		// Right side - Third pipeline
		pipeline3.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer, false);
	}

private:
	Pipeline pipeline1, pipeline2, pipeline3;
	Descriptor descriptor;
	Model model;
	UBO<UBO_lightToon> lightToonUBO;	// For scene 2
};

DECLARE_APP(PipelinesMultipleDerived)
