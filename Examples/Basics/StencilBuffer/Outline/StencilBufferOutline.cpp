#include <VulkanPlayground\Includes.h>

class StencilBufferOutlineApp : public VulkanApplication3D
{
	bool UseStencilBuffer() override { return true; }


	void ResetScene() override
	{
		CalcPositionMatrix({ 0, 180.0f, 0.0f }, { -3.8f, -0.5f, 0 }, 0, 0, model.GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "venus.fbx"), Attribs::PosNorm);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		outlineDescriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		CreateDescriptors(system, { &descriptor, &outlineDescriptor }, "Main");

		pipeline.EnableStencilTest(VK_COMPARE_OP_ALWAYS, VK_STENCIL_OP_REPLACE);
		pipeline.SetupVertexDescription(Attribs::PosNorm);
		pipeline.LoadShaderDiffNames(system, "StencilN");	// Only a vertex shader to write stencil values
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Stencil");

		outlinePipeline.EnableStencilTest(VK_COMPARE_OP_NOT_EQUAL, VK_STENCIL_OP_KEEP);
		outlinePipeline.SetupVertexDescription(Attribs::PosNorm);
		outlinePipeline.LoadShader(system, "Outline");
		CreatePipeline(system, renderPass, outlinePipeline, outlineDescriptor, workingExtent, "Outline");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);

		outlinePipeline.Bind(commandBuffer, outlineDescriptor);
		model.Draw(commandBuffer);
	}

private:
	Descriptor descriptor, outlineDescriptor;
	Pipeline pipeline, outlinePipeline;
	Model model;
	UBO<MVP> uniformBuffer;
};

DECLARE_APP(StencilBufferOutline)