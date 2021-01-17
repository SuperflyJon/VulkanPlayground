#include <VulkanPlayground\Includes.h>

class StencilBufferStencilApp : public VulkanApplication3D
{
	bool UseStencilBuffer() override { return true; }

	void ResetScene() override
	{
		CalcPositionMatrix({ 0, 180.0f, 0.0f }, { -3.8f, -0.8f, 0 }, 0, 10, model.GetModelSize());
	}

	void SetupRenderPasses(RenderPasses& renderPasses, VulkanSystem& system, VkFormat imageFormat) override
	{
		RenderPass& renderPass = renderPasses.NewRenderPass();
		// First sub-pass, populate stencil buffer attachment (no colour attachment)
		renderPass.GetDisplayBuffers().SetDepthImageAspect(VK_IMAGE_ASPECT_STENCIL_BIT);	// We want to view the stencil buffer
		renderPass.SetDepthStencilClearValue({ 1.0f, 50 });	// Clear to 50 for testing
		stencilAttach = renderPass.AddDepthAttachment(GetDepthFormat(system));

		// Second sub-pass, display stencil buffer on screen
		renderPass.AddNewSubPass();
		renderPass.AddColourAttachment(imageFormat, "MainBuffer", VK_ATTACHMENT_LOAD_OP_DONT_CARE);
		renderPass.AddInput(stencilAttach);	// Add stencil buffer as input to this subpass
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "venus.fbx"), Attribs::Pos);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		readDescriptor.AddSubPassInput(0, renderPass.GetAttachmentImage(stencilAttach));
		CreateDescriptors(system, { &descriptor, &readDescriptor }, "Drawing");

		pipeline.EnableStencilTest(VK_COMPARE_OP_ALWAYS, VK_STENCIL_OP_REPLACE, 200);	// Set value to 200 - shows as light grey
		pipeline.SetupVertexDescription(Attribs::Pos);
		pipeline.LoadShaderDiffNames(system, "Stencil");	// Only a vertex shader

		pipelineRead.LoadShader(system, "ReadAttach");

		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "StencilWrite");
		pipelineRead.SetSubpass(1);
		CreatePipeline(system, renderPass, pipelineRead, readDescriptor, workingExtent, "ReadStencil");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);

		vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
		pipelineRead.Bind(commandBuffer, readDescriptor);
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	}

private:
	Descriptor descriptor, readDescriptor;
	Pipeline pipeline, pipelineRead;
	Model model;
	uint32_t stencilAttach = 0;
};

DECLARE_APP(StencilBufferStencil)
