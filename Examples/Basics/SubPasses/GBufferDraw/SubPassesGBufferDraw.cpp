#include <VulkanPlayground\Includes.h>

class SubPassesGBufferDrawApp : public VulkanApplication3DLight
{
	void ResetScene() override
	{
		CalcPositionMatrix({ 0.0f, 15.0f, 0.0f }, { -0.3, -0.1, -0.05 }, 0, 0, model.GetModelSize());
		SetupLighting({ 1.0f, -0.8f, -0.8f }, 0.5f, 0.7f, 0.3f, 32.0f, model.GetModelSize());
	}
	
	void SetupRenderPasses(RenderPasses& renderPasses, VulkanSystem& system, VkFormat imageFormat) override
	{
		RenderPass& renderPass = renderPasses.NewRenderPass();
		// First sub-pass, render off screen
		positionAttach = renderPass.AddIntermidiateColourAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, "Position");
		albedoAttach = renderPass.AddIntermidiateColourAttachment(VK_FORMAT_R8G8B8A8_UNORM, "Albedo");
		normalAttach = renderPass.AddIntermidiateColourAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, "Normal");
		renderPass.AddDepthAttachment(GetDepthFormat(system));

		// Second sub-pass, display gbuffer components on screen
		renderPass.AddNewSubPass();
		uint32_t colourAttach = renderPass.AddColourAttachment(imageFormat, "MainBuffer", VK_ATTACHMENT_LOAD_OP_DONT_CARE);
		renderPass.AddInput(positionAttach);
		renderPass.AddInput(albedoAttach);
		renderPass.AddInput(normalAttach);

		// Third sub-pass, display gbuffer components on screen
		renderPass.AddNewSubPass();
		renderPass.AddExistingColourAttachment(colourAttach);
		renderPass.AddInput(positionAttach);
		renderPass.AddInput(albedoAttach);
		renderPass.AddInput(normalAttach);
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "samplebuilding.dae"), Attribs::PosNormCol);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");

		readDescriptor.AddUniformBuffer(system, 3, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);
		readDescriptor.AddSubPassInput(0, renderPass.GetAttachmentImage(positionAttach));
		readDescriptor.AddSubPassInput(1, renderPass.GetAttachmentImage(albedoAttach));
		readDescriptor.AddSubPassInput(2, renderPass.GetAttachmentImage(normalAttach));

		testDescriptor.AddSubPassInput(0, renderPass.GetAttachmentImage(positionAttach));
		testDescriptor.AddSubPassInput(1, renderPass.GetAttachmentImage(albedoAttach));
		testDescriptor.AddSubPassInput(2, renderPass.GetAttachmentImage(normalAttach));

		CreateDescriptors(system, { &descriptor, &readDescriptor, &testDescriptor }, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		pipeline.LoadShaderDiffNames(system, "spScene", "GBufferDraw");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "FirstPass");

		pipelineRead.SetSubpass(1);
		pipelineRead.LoadShader(system, "Composition");
		CreatePipeline(system, renderPass, pipelineRead, readDescriptor, workingExtent, "SecondPass");

		pipelineTest.SetSubpass(2);
		pipelineTest.LoadShaderDiffNames(system, "Composition", "TestReadBuffers");
		pipelineTest.AddPushConstant(system, sizeof(attachType), VK_SHADER_STAGE_FRAGMENT_BIT);
		CreatePipeline(system, renderPass, pipelineTest, testDescriptor, workingExtent, "ThirdPass");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipelineTest.PushConstant(commandBuffer, &attachType);

		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);

		vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
		pipelineRead.Bind(commandBuffer, readDescriptor.GetDescriptorSet());
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);

		vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
		pipelineTest.Bind(commandBuffer, testDescriptor.GetDescriptorSet());
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			attachType = (attachType + 1) % 4;
			pipelineTest.Recreate();	// Recreate pipeline to use new constant value
		}
	}

private:
	Descriptor descriptor, readDescriptor, testDescriptor;
	Pipeline pipeline, pipelineRead, pipelineTest;
	Model model;
	
	uint32_t positionAttach, normalAttach, albedoAttach;
	int attachType = 0;
};

DECLARE_APP(SubPassesGBufferDraw)
