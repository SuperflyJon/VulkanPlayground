#include <VulkanPlayground\Includes.h>

class DynamicUniformBufferMultipleCubesApp : public VulkanApplication3D
{
	void ResetScene() override
	{
		numCubes = 2;
		UpdatePos();
	}
	void UpdatePos()
	{
		CalcPositionMatrixMoveBack(numCubes * 2.0f, model.GetModelSize());
		worldPos.Reset({(numCubes - 1) * 2, 0, 0 }, 0, 0);	// Move inbetween 2 cubes
		RecreateObjects();
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "cube.obj"), Attribs::PosTex);
		if (numCubes == 0)
			return;
		descriptor.AddDynamicUniformBuffer(system, 0, uniformBuffer, numCubes, "MVP");
		descriptor.AddTexture(system, 1, texture, VulkanPlayground::GetModelFile("Basics", "crate01_color_height_rgba.ktx"));
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.LoadShader(system, "cube");
		pipeline.SetupVertexDescription(Attribs::PosTex);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");
	};

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		for (uint32_t buff = 0; buff < numCubes; buff++)
		{
			pipeline.Bind(commandBuffer, descriptor.GetDescriptorSet(), buff * uniformBuffer.GetBlockSize());
			model.Draw(commandBuffer, (buff == 0));
		}
	}

	void UpdateScene(VulkanSystem& /*system*/, float /*frameTime*/) override
	{
		for (uint32_t buff = 0; buff < numCubes; buff++)
		{
			uniformBuffer(buff).projection = mvpUBO().projection;
			uniformBuffer(buff).view = mvpUBO().view;
			uniformBuffer(buff).model = glm::translate(mvpUBO().model, glm::vec3(4.0f * buff, 0.0f, 0.0f));
		}
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			if (eventData.ShiftPressed())
			{
				if (numCubes > 1)
					numCubes--;
			}
			else
			{
				if (numCubes < 25)
					numCubes++;
			}
			UpdatePos();
		}
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Texture texture;
	DynamicUBO<MVP> uniformBuffer;
	Model model;
	uint32_t numCubes = 0;
};

DECLARE_APP(DynamicUniformBufferMultipleCubes)
