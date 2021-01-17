#include <VulkanPlayground\Includes.h>
#include <random>
#include <glm\gtx\rotate_vector.hpp>

class PushConstantsCubesApp : public VulkanApplication3DLight
{
	struct UBO_mr
	{
		glm::vec3 rotation;
		float pad;
		glm::vec3 offset;
	};

	void ResetScene() override
	{
		spinCubes = true;
		cubeDimension = 10;
		UpdatePos();
	}
	void UpdatePos()
	{
		totalTime = 0;
		cubeSize = model.GetModelSize().x;
		CalcPositionMatrixMoveBack(cubeDimension * cubeSize, glm::vec3(cubeSize));
		worldPos.Reset(glm::vec3((cubeDimension - 1) * cubeSize), 0, 0);
		SetupLighting({ 0.4, -0.7, 0 }, 0.5f, 0.8f, 0.20f, 50.0f, glm::vec3(cubeSize));
		RecreateObjects();
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "cube.obj"), Attribs::PosNormTex);
		if (cubeDimension == 0)
			return;

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddTexture(system, 3, texture, VulkanPlayground::GetModelFile("Basics", "crate01_color_height_rgba.ktx"));
		descriptor.AddUniformBuffer(system, 4, lightUBO, "Light", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosNormTex);
		pipeline.LoadShader(system, "cubePCLit");
		pipeline.AddPushConstant(system, sizeof(UBO_mr), VK_SHADER_STAGE_VERTEX_BIT);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");

		// Set cube position offset and rotate amounts
		std::default_random_engine rndEngine(std::random_device{}());
		std::normal_distribution<float> rndDist(-2.0f * MathsContants::pi<float>, 2.0f * MathsContants::pi<float>);
		uint32_t index = 0;
		delete[] cubeInfo;
		cubeInfo = new UBO_mr[cubeDimension * cubeDimension * cubeDimension];
		for (uint32_t x = 0; x < cubeDimension; x++)
		{
			for (uint32_t y = 0; y < cubeDimension; y++)
			{
				for (uint32_t z = 0; z < cubeDimension; z++)
				{
					cubeInfo[index].rotation = glm::vec3(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine));
					cubeInfo[index].offset = glm::vec3(x, y, z) * glm::vec3(cubeSize * 1.8f);
					index++;
				}
			}
		}
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		for (uint32_t buff = 0; buff < cubeDimension * cubeDimension * cubeDimension; buff++)
		{
			UBO_mr cubePos = cubeInfo[buff];
			cubePos.rotation *= totalTime / 25;
			pipeline.PushConstant(commandBuffer, &cubePos);

			pipeline.Bind(commandBuffer, descriptor);
			model.Draw(commandBuffer, (buff == 0));
		}
	}

	void UpdateScene(VulkanSystem& /*system*/, float frameTime) override
	{
		if (spinCubes)
		{
			totalTime += frameTime;

			RedrawScene();	// Push constant passed via commanbuffer
		}
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
		if (eventData.KeyPressed(' '))
		{
			spinCubes = !spinCubes;
		}
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Model model;
	Texture texture;
	UBO_mr* cubeInfo = nullptr;
	float totalTime;
	uint32_t cubeDimension = 0;
	bool spinCubes;
	float cubeSize;
};

DECLARE_APP(PushConstantsCubes)
