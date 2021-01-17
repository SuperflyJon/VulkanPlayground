#include <VulkanPlayground\Includes.h>

class DescriptorSetsLitCubesApp : public VulkanApplication3DLight
{
	void ResetScene() override
	{
		CalcPositionMatrixMoveBack(2, model.GetModelSize());
		worldPos.Reset({ 2, 0, 0 }, 0, 0);	// Move inbetween 2 cubes
		SetupLighting({ 0.4, -0.7, 0 }, 0.5f, 0.8f, 0.20f, 50.0f, model.GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "cube.obj"), Attribs::PosNormTex);

		descriptor1.AddUniformBuffer(system, 0, mvpUBO, "MVP1");
		descriptor1.AddUniformBuffer(system, 2, lightUBO, "Light Info", VK_SHADER_STAGE_FRAGMENT_BIT);
		descriptor1.AddTexture(system, 1, texture1, VulkanPlayground::GetModelFile("Basics", "crate01_color_height_rgba.ktx"));
		CreateBaseDescriptor(system, descriptor1, "Drawing1", 2);

		descriptor2.AddUniformBuffer(system, 0, uniformBuffer2, "MVP2");
		descriptor2.AddUniformBuffer(system, 2, lightUBO, "Light Info", VK_SHADER_STAGE_FRAGMENT_BIT);
		descriptor2.AddTexture(system, 1, texture2, VulkanPlayground::GetModelFile("Basics", "crate02_color_height_rgba.ktx"));
		CreateSharedDescriptor(system, descriptor2, descriptor1.GetDescriptorSetLayout(), "Drawing2");

		pipeline.SetupVertexDescription(Attribs::PosNormTex);
		pipeline.LoadShader(system, "cubelit");
		CreatePipeline(system, renderPass, pipeline, descriptor1, workingExtent, "Scene");
	};

	void UpdateScene(VulkanSystem& system, float /*frameTime*/) override
	{
		uniformBuffer2().projection = mvpUBO().projection;
		uniformBuffer2().view = mvpUBO().view;
		uniformBuffer2().model = glm::translate(mvpUBO().model, glm::vec3(4.0f, 0.0f, 0.0f));	// Move 2nd cube away from first one
		uniformBuffer2.CopyToDevice(system);
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor1);
		model.Draw(commandBuffer);

		pipeline.Bind(commandBuffer, descriptor2);
		model.Draw(commandBuffer);
	}

private:
	Pipeline pipeline;
	Descriptor descriptor1, descriptor2;
	Texture texture1, texture2;
	UBO<MVP> uniformBuffer2;
	Model model;
};

DECLARE_APP(DescriptorSetsLitCubes)
