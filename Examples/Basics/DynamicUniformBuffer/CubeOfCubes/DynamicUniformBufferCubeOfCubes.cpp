#include <VulkanPlayground\Includes.h>

class DynamicUniformBufferCubeOfCubesApp : public VulkanApplication3D
{
	struct UBO_vp
	{
		glm::mat4 view;
		glm::mat4 projection;
	};
	struct UBO_m
	{
		glm::mat4 model;
	};

	void ResetScene() override
	{
		cubeDimension = 2;
		UpdatePos();
	}
	void UpdatePos()
	{
		CalcPositionMatrixMoveBack(cubeDimension * 2.0f, model.GetModelSize());
		worldPos.Reset(glm::vec3((cubeDimension - 1) * 1.25f), 0, 0);
		RecreateObjects();
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "cube.obj"), Attribs::PosNormTex);
		if (cubeDimension == 0)
			return;

		descriptor.AddUniformBuffer(system, 0, uniformBuffer, "VP");
		descriptor.AddDynamicUniformBuffer(system, 1, dynamicUniformBuffer, cubeDimension * cubeDimension * cubeDimension, "Model");
		descriptor.AddTexture(system, 2, texture, VulkanPlayground::GetModelFile("Basics", "crate01_color_height_rgba.ktx"));
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosNormTex);
		pipeline.LoadShader(system, "cubeDynUbo");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");
	};

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		for (uint32_t buff = 0; buff < dynamicUniformBuffer.GetNumDynamicBuffers(); buff++)
		{
			pipeline.Bind(commandBuffer, descriptor.GetDescriptorSet(), buff * dynamicUniformBuffer.GetBlockSize());
			model.Draw(commandBuffer, (buff == 0));
		}
	}

	void UpdateScene(VulkanSystem& system, float /*frameTime*/) override
	{
		uniformBuffer().projection = mvpUBO().projection;
		uniformBuffer().view = mvpUBO().view;
		uniformBuffer.CopyToDevice(system);

		// Dynamic ubo with per-object model matrices indexed by offsets in the command buffer
		for (uint32_t x = 0; x < cubeDimension; x++)
		{
			for (uint32_t y = 0; y < cubeDimension; y++)
			{
				for (uint32_t z = 0; z < cubeDimension; z++)
				{
					uint32_t index = (x * cubeDimension * cubeDimension) + (y * cubeDimension) + z;
					dynamicUniformBuffer(index).model = glm::translate(mvpUBO().model, glm::vec3(x * 2.5, y * 2.5, z * 2.5));
				}
			}
		}
		dynamicUniformBuffer.CopyToDevice(system);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			if (eventData.ShiftPressed())
			{
				if (cubeDimension > 1)
					cubeDimension--;
			}
			else
			{
				if (cubeDimension < 25)
					cubeDimension++;
			}
			UpdatePos();
		}
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Model model;
	Texture texture;
	UBO<UBO_vp> uniformBuffer;
	DynamicUBO<UBO_m> dynamicUniformBuffer;
	uint32_t cubeDimension = 0;
};

DECLARE_APP(DynamicUniformBufferCubeOfCubes)
