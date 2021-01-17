#include <VulkanPlayground\Includes.h>

class InputAttachmentsDepthApp : public VulkanApplication3D
{
	void ResetScene() override
	{
		CalcPositionMatrix({ 0, -180, 0 }, { -1, -1.5, 0 }, 0, 30, model.GetModelSize());
	}

	void SetupRenderPasses(RenderPasses& renderPasses, VulkanSystem& system, VkFormat imageFormat) override
	{
		RenderPass& renderPass = renderPasses.NewRenderPass();
		// First sub-pass, populate depth buffer attachment (no colour attachment)
		depthAttach = renderPass.AddDepthAttachment(GetDepthFormat(system));
		// Second sub-pass, display depth values on screen
		renderPass.AddNewSubPass();
		renderPass.AddColourAttachment(imageFormat, "MainBuffer", VK_ATTACHMENT_LOAD_OP_DONT_CARE);
		renderPass.AddInput(depthAttach);	// Add depth buffer as input to this subpass
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "treasure_smooth.dae"), Attribs::Pos);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		readDescriptor.AddSubPassInput(0, renderPass.GetAttachmentImage(depthAttach));
		CreateDescriptors(system, { &descriptor, &readDescriptor }, "Drawing");

		pipeline.SetupVertexDescription(Attribs::Pos);
		pipeline.LoadShaderDiffNames(system, "Depth");	// Only a vertex shader
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "DepthWrite");

		pipelineRead.SetSubpass(1);
		pipelineRead.LoadShader(system, "ReadDepthAttach");
		CreatePipeline(system, renderPass, pipelineRead, readDescriptor, workingExtent, "ReadDepth");
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
	uint32_t depthAttach = 0;
};

DECLARE_APP(InputAttachmentsDepth)
