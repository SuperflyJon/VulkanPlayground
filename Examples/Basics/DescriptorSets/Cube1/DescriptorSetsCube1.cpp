#include <VulkanPlayground\Includes.h>

class DescriptorSetsCube1App : public VulkanApplication3D
{
	void ResetScene() override
	{
		CalcPositionMatrixMoveBack(2, model.GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "cube.obj"), Attribs::PosTex);
		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddTexture(system, 1, texture, VulkanPlayground::GetModelFile("Basics", "crate01_color_height_rgba.ktx"));
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosTex);
		pipeline.LoadShader(system, "cube");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Texture texture;
	Model model;
};

DECLARE_APP(DescriptorSetsCube1)
