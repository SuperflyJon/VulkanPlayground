#include <VulkanPlayground\Includes.h>

class ModelTestMeshApp : public VulkanApplication3D
{
	void ResetScene() override
	{
		CalcPositionMatrix({ 0, 20, 0 }, { -1.0f, -0.4f, 0 }, 0, 25, model.GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "voyager.dae"), Attribs::PosTex);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddTexture(system, 1, texture, VulkanPlayground::GetModelFile("Basics", "voyager_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosTex);
		pipeline.LoadShader(system, "Model");
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

DECLARE_APP(ModelTestMesh)
