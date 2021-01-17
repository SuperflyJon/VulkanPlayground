#include <VulkanPlayground\Includes.h>

class InputAttachmentsSceneApp : public VulkanApplication3D
{
	bool UseDepthBuffer() override { return useDepthBuffer; }

	void ResetScene() override
	{
		CalcPositionMatrix({ 0, -180, 0 }, { -1, -1.5, 0 }, 0, 30, model.GetModelSize());
		useDepthBuffer = true;
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "treasure_smooth.dae"), Attribs::PosNormCol);
		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		pipeline.LoadShader(system, "Scene");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			useDepthBuffer = !useDepthBuffer;
			RecreateObjects();
		}
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Model model;
	bool useDepthBuffer = true;
};

DECLARE_APP(InputAttachmentsScene)
