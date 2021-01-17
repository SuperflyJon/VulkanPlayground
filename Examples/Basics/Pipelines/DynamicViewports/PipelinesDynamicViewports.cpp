#include <VulkanPlayground\Includes.h>

class PipelinesDynamicViewportsApp : public VulkanApplication3D
{
	void GetRequiredDeviceFeatures(const VkPhysicalDeviceFeatures& deviceFeatures, VkPhysicalDeviceFeatures* requiredFeatures) override
	{
		VulkanPlayground::EnableFillModeNonSolid(deviceFeatures, requiredFeatures);
		VulkanPlayground::EnableWideLines(deviceFeatures, requiredFeatures);
	}

	void ResetScene() override
	{
		CalcPositionMatrix({ 0, -180, 0 }, { -1, -1.5, 0 }, 0, 30, model.GetModelSize());
		midSize = 1 / 3.0f;
		RedrawScene();
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		VulkanPlayground::SetupProjectionMatrix(mvpUBO().projection, AspectRatio() / 3);

		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "treasure_smooth.dae"), Attribs::PosNormCol);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.EnableDynamicState(VK_DYNAMIC_STATE_LINE_WIDTH);
		pipeline.EnableDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		pipeline.LoadShader(system, "PipelinesScene3");
		pipeline.SetPolygonMode(VK_POLYGON_MODE_LINE);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "PipelinesScene3");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		float midWidth = (float)GetWindowWidth() * midSize;
		float otherWidth = ((float)GetWindowWidth() - midWidth) / 2;
		float height = (float)GetWindowHeight();

		pipeline.Bind(commandBuffer, descriptor);

		// Left side - First viewport
		vkCmdSetLineWidth(commandBuffer, 1.f);
		VulkanPlayground::SetViewport(commandBuffer, 0, 0, otherWidth, height);
		model.Draw(commandBuffer);

		// Middle - Second viewport
		vkCmdSetLineWidth(commandBuffer, 2.f);
		VulkanPlayground::SetViewport(commandBuffer, otherWidth, 0, midWidth, height);
		model.Draw(commandBuffer, false);

		// Right side - Third viewport
		VulkanPlayground::SetViewport(commandBuffer, otherWidth + midWidth, 0, otherWidth, height);
		vkCmdSetLineWidth(commandBuffer, 10.0f);
		model.Draw(commandBuffer, false);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			if (eventData.ShiftPressed())
			{
				midSize -= .1f;
				if (midSize < .1f)
					midSize = .1f;
			}
			else
			{
				midSize += .1f;
				if (midSize > 0.9f)
					midSize = 0.9f;
			}
			RedrawScene();
		}
	}

private:
	float midSize;
	Pipeline pipeline;
	Descriptor descriptor;
	Model model;
};

DECLARE_APP(PipelinesDynamicViewports)
