#include <VulkanPlayground\Includes.h>

class PipelinesScene3App : public VulkanApplication3D
{
	void GetRequiredDeviceFeatures(const VkPhysicalDeviceFeatures& deviceFeatures, VkPhysicalDeviceFeatures* requiredFeatures) override
	{
		VulkanPlayground::EnableFillModeNonSolid(deviceFeatures, requiredFeatures);
		VulkanPlayground::EnableWideLines(deviceFeatures, requiredFeatures);
	}

	void ResetScene() override
	{
		CalcPositionMatrix({ 0, -180, 0 }, { -1, -1.5, 0 }, 0, 30, model.GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "treasure_smooth.dae"), Attribs::PosNormCol);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.EnableDynamicState(VK_DYNAMIC_STATE_LINE_WIDTH);
		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		pipeline.LoadShader(system, "PipelinesScene3");
		pipeline.SetPolygonMode(VK_POLYGON_MODE_LINE);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "PipelinesScene3");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		vkCmdSetLineWidth(commandBuffer, 2.0f);
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Model model;
};

DECLARE_APP(PipelinesScene3)
