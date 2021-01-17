#include <VulkanPlayground\Includes.h>

class ModelTestModelApp : public VulkanApplication3D
{
	void ResetScene() override
	{
		cameraPos.Reset(model.GetModelSize() * glm::vec3(-0.8f, -0.5f, 0), 0, 25);
	}

	void UpdateScene(VulkanSystem& /*system*/, float /*frameTime*/) override
	{
		mvpUBO().model = glm::rotate(mvpUBO().model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		mvpUBO().model = glm::rotate(mvpUBO().model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
#ifdef _DEBUG
		model.DontUseIndicies();	// Auto creating indicies a bit slow in debug...
#endif
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "chalet.obj"), Attribs::PosTex);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddTexture(system, 1, texture, VulkanPlayground::GetModelFile("Basics", "chalet.jpg"));
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosTex);
		pipeline.LoadShader(system, "model");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "TestModel");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Model model;
	Texture texture;
};

DECLARE_APP(ModelTestModel)
