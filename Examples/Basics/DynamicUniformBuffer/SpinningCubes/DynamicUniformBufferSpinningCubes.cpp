#include <VulkanPlayground\Includes.h>
#include <random>

class DynamicUniformBufferSpinningCubesApp : public VulkanApplication3D
{
	static const int cubeDimension = 5;

	struct UBO_vp
	{
		glm::mat4 view;
		glm::mat4 projection;
	};
	struct UBO_mr
	{
		glm::mat4 model;
		glm::mat4 rotation;
	};

	void ResetScene() override
	{
		totalTime = 0;

		std::default_random_engine rndEngine(std::random_device{}());
		std::normal_distribution<float> rndDist(-2.0f * MathsContants::pi<float>, 2.0f * MathsContants::pi<float>);
		for (uint32_t i = 0; i < cubeDimension * cubeDimension * cubeDimension; i++)
		{
			rotations[i] = glm::vec3(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine));
		}
		float cubeSize = model.GetModelSize().x;
		CalcPositionMatrixMoveBack(cubeDimension * cubeSize, glm::vec3(cubeSize));
		worldPos.Reset(glm::vec3(cubeDimension * cubeSize), 0, 0);
		RecreateObjects();
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "cube.obj"), Attribs::PosTex);

		descriptor.AddUniformBuffer(system, 0, uniformBuffer, "VP");
		descriptor.AddDynamicUniformBuffer(system, 1, dynamicUniformBuffer, cubeDimension * cubeDimension * cubeDimension, "Model");
		descriptor.AddTexture(system, 2, texture, VulkanPlayground::GetModelFile("Basics", "crate01_color_height_rgba.ktx"));
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosTex);
		pipeline.LoadShader(system, "cubeDynUboSpin");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		for (uint32_t buff = 0; buff < dynamicUniformBuffer.GetNumDynamicBuffers(); buff++)
		{
			pipeline.Bind(commandBuffer, descriptor.GetDescriptorSet(), buff * dynamicUniformBuffer.GetBlockSize());
			model.Draw(commandBuffer, (buff == 0));
		}
	}

	void UpdateScene(VulkanSystem& system, float frameTime) override
	{
		uniformBuffer().projection = mvpUBO().projection;
		uniformBuffer().view = mvpUBO().view;
		uniformBuffer.CopyToDevice(system);

		totalTime += frameTime;

		float rotateAmount = totalTime / 25;
		uint32_t index = 0;
		for (uint32_t x = 0; x < cubeDimension; x++)
		{
			for (uint32_t y = 0; y < cubeDimension; y++)
			{
				for (uint32_t z = 0; z < cubeDimension; z++)
				{
					glm::mat4 rotation;
					rotation = glm::rotate(rotation, rotations[index].x * rotateAmount, glm::vec3(1.0f, 0.0f, 0.0f));
					rotation = glm::rotate(rotation, rotations[index].y * rotateAmount, glm::vec3(0.0f, 1.0f, 0.0f));
					rotation = glm::rotate(rotation, rotations[index].z * rotateAmount, glm::vec3(0.0f, 0.0f, 1.0f));
					dynamicUniformBuffer(index).rotation = rotation;
					dynamicUniformBuffer(index).model = glm::translate(mvpUBO().model, glm::vec3(x * cubeDimension, y * cubeDimension, z * cubeDimension));
					index++;
				}
			}
		}
		dynamicUniformBuffer.CopyToDevice(system);
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Model model;
	Texture texture;
	UBO<UBO_vp> uniformBuffer;
	DynamicUBO<UBO_mr> dynamicUniformBuffer;
	glm::vec3 rotations[cubeDimension * cubeDimension * cubeDimension];
	float totalTime;
};

DECLARE_APP(DynamicUniformBufferSpinningCubes)
