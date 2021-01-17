#include <VulkanPlayground\Includes.h>
#include <glm\gtx\rotate_vector.hpp>

class PushConstantsUniformSpotApp : public VulkanApplication3DLight
{
	struct UBO_spotlight
	{
		glm::vec4 lookatPos;
		int hardSpot;
		int showSpot;
	};

	void ResetScene() override
	{
		rotate = true;
		rotateAmount = 0.0f;
		spotUBO().showSpot = true;
		spotUBO().hardSpot = false;
		CalcPositionMatrix({ 0, 225, 0 }, { -0.8, -0.8, 0 }, 0, 20, model.GetModelSize());
		SetupLighting({ -0.5f, -1, -0.3f }, 0.5f, 0.8f, 0.2f, 30.0f, model.GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "SampleScene.dae"), Attribs::PosNormCol);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 2, lightUBO, "Lights", VK_SHADER_STAGE_FRAGMENT_BIT);
		descriptor.AddUniformBuffer(system, 3, spotUBO, "Spot", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		pipeline.LoadShaderDiffNames(system, "SampleModel", "UniformSpot");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed(' '))
		{
			rotate = !rotate;
		}
		if (eventData.KeyPressed('T'))
		{
			spotUBO().showSpot = !spotUBO().showSpot;
		}
		if (eventData.KeyPressed('H'))
		{
			spotUBO().hardSpot = !spotUBO().hardSpot;
		}
	}

	void UpdateScene(VulkanSystem& system, float frameTime) override
	{
		if (rotate)
		{
			rotateAmount += frameTime;
			spotUBO().lookatPos = glm::vec4(glm::rotateY(glm::vec3(0.0f, 0.0f, 10.0f), rotateAmount * 3), cos(glm::radians(10.0f)));
		}
		spotUBO.CopyToDevice(system);
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Model model;
	UBO<UBO_spotlight> spotUBO;
	bool rotate;
	float rotateAmount;
};

DECLARE_APP(PushConstantsUniformSpot)
