#include <VulkanPlayground\Includes.h>

constexpr auto NUM_ATTACHMENTS = 6;

class SubPassesGBufferApp : public VulkanApplication3DLight
{
	void GetRequiredDeviceFeatures(const VkPhysicalDeviceFeatures& deviceFeatures, VkPhysicalDeviceFeatures* requiredFeatures) override
	{
		if (deviceFeatures.independentBlend)
			requiredFeatures->independentBlend = VK_TRUE;	// Mixture of attachement types
		else
			throw std::runtime_error("Independent blend not supported!");
	}

	void ResetScene() override
	{
		CalcPositionMatrix({ 0.0f, 15.0f, 0.0f }, { -0.3, -0.1, -0.05 }, 0, 0, model.GetModelSize());
		SetupLighting({ 1.0f, -0.8f, -0.8f }, 0.5f, 0.7f, 0.3f, 32.0f, model.GetModelSize());
	}

	void SetupRenderPasses(RenderPasses& renderPasses, VulkanSystem& system, VkFormat imageFormat) override
	{
		RenderPass& renderPass = renderPasses.NewRenderPass();
		// First sub-pass, render off screen
		colourAttach = renderPass.AddIntermidiateColourAttachment(VulkanPlayground::offscreenColourBufferFormat, "Colour");
		positionAttach = renderPass.AddIntermidiateColourAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, "Position");
		albedoAttach = renderPass.AddIntermidiateColourAttachment(VK_FORMAT_R8G8B8A8_UNORM, "Albedo");
		normalAttach = renderPass.AddIntermidiateColourAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, "Normal");
		depthAttach = renderPass.AddDepthAttachment(GetDepthFormat(system));

		// Second sub-pass, display gbuffer components on screen
		renderPass.AddNewSubPass();
		renderPass.AddColourAttachment(imageFormat, "MainBuffer", VK_ATTACHMENT_LOAD_OP_DONT_CARE);
		renderPass.AddInput(colourAttach);
		renderPass.AddInput(positionAttach);
		renderPass.AddInput(albedoAttach);
		renderPass.AddInput(normalAttach);
		renderPass.AddInput(depthAttach);
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "samplebuilding.dae"), Attribs::PosNormCol);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 2, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);

		readDescriptor.AddSubPassInput(0, renderPass.GetAttachmentImage(colourAttach));
		readDescriptor.AddSubPassInput(1, renderPass.GetAttachmentImage(positionAttach));
		readDescriptor.AddSubPassInput(2, renderPass.GetAttachmentImage(albedoAttach));
		readDescriptor.AddSubPassInput(3, renderPass.GetAttachmentImage(normalAttach));
		readDescriptor.AddSubPassInput(4, renderPass.GetAttachmentImage(depthAttach));

		CreateDescriptors(system, { &descriptor, &readDescriptor }, "Drawing");

		const float nearPlane = 0.1f;
		const float farPlane = 256.0f;

		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		Shader& gbufferShader = pipeline.LoadShaderDiffNames(system, "spScene", "GBuffer");
		gbufferShader.SetSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, nearPlane);
		gbufferShader.SetSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 1, farPlane);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "FirstPass");

		pipelineRead.LoadShader(system, "GBufReadAttach");
		pipelineRead.SetSubpass(1);
		pipelineRead.AddPushConstant(system, sizeof(attachType), VK_SHADER_STAGE_FRAGMENT_BIT);
		CreatePipeline(system, renderPass, pipelineRead, readDescriptor, workingExtent, "SecondPass");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipelineRead.PushConstant(commandBuffer, &attachType);

		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);

		vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
		pipelineRead.Bind(commandBuffer, readDescriptor.GetDescriptorSet());
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			attachType = (attachType + 1) % (NUM_ATTACHMENTS + 1);
			// Force pipeline regeneration (to use new constant value)
			pipeline.Recreate();
			pipelineRead.Recreate();
		}
	}

private:
	Descriptor descriptor, readDescriptor;
	Pipeline pipeline, pipelineRead;
	Model model;
	
	uint32_t colourAttach, positionAttach, albedoAttach, normalAttach, depthAttach;
	int attachType = NUM_ATTACHMENTS;
};

DECLARE_APP(SubPassesGBuffer)
