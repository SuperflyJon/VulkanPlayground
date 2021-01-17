#include <VulkanPlayground\Includes.h>

class InputAttachmentsMultipassApp : public VulkanApplication3D
{
	void ResetScene() override
	{
		CalcPositionMatrix({ 0, -180, 0 }, { -1, -1.5, 0 }, 0, 30, model.GetModelSize());
	}

	void SetupRenderPasses(RenderPasses& renderPasses, VulkanSystem& system, VkFormat imageFormat) override
	{
		RenderPass& renderPass = renderPasses.NewRenderPass();

		// First sub-pass, render off screen
		colourAttach = renderPass.AddIntermidiateColourAttachment(VulkanPlayground::offscreenColourBufferFormat, "FullBuffer");
		depthAttach = renderPass.AddDepthAttachment(GetDepthFormat(system));

		// Second sub-pass, read attachments
		renderPass.AddNewSubPass();
		renderPass.AddColourAttachment(imageFormat, "MainBuffer", VK_ATTACHMENT_LOAD_OP_DONT_CARE);
		renderPass.AddInput(colourAttach);	// Add colour buffer as input to this subpass
		renderPass.AddInput(depthAttach);	// Add depth buffer as input to this subpass
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "treasure_smooth.dae"), Attribs::PosNormCol);
		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");

		readDescriptor.AddSubPassInput(0, renderPass.GetAttachmentImage(colourAttach));
		readDescriptor.AddSubPassInput(1, renderPass.GetAttachmentImage(depthAttach));

		CreateDescriptors(system, { &descriptor, &readDescriptor }, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		pipeline.LoadShader(system, "Scene");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");

		pipelineRead.SetSubpass(1);
		pipelineRead.LoadShader(system, "MultiReadAttach");
		CreatePipeline(system, renderPass, pipelineRead, readDescriptor, workingExtent, "ReadPass");
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
	Pipeline pipeline, pipelineRead;
	Descriptor descriptor, readDescriptor;
	Model model;
	uint32_t depthAttach, colourAttach;
};

DECLARE_APP(InputAttachmentsMultipass)
