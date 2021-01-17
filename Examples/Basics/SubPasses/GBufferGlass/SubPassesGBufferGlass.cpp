#include <VulkanPlayground\Includes.h>

class SubPassesGBufferGlassApp : public VulkanApplication3DLight
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

		// Third sub-pass, draw glass windows
		renderPass.AddNewSubPass();
		renderPass.AddExistingColourAttachment(colourAttach);
		renderPass.AddInput(positionAttach);	// Use linear depth from alpha component
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		const float nearPlane = 0.1f;
		const float farPlane = 256.0f;
	
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "samplebuilding.dae"), Attribs::PosNormCol);
		glassModel.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "samplebuilding_glass.dae"), Attribs::PosTex);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");

		readDescriptor.AddUniformBuffer(system, 3, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);
		readDescriptor.AddSubPassInput(0, renderPass.GetAttachmentImage(positionAttach));
		readDescriptor.AddSubPassInput(1, renderPass.GetAttachmentImage(albedoAttach));
		readDescriptor.AddSubPassInput(2, renderPass.GetAttachmentImage(normalAttach));

		glassDescriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		glassDescriptor.AddTexture(system, 2, texture, VulkanPlayground::GetModelFile("Basics", "coloured_glass_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		glassDescriptor.AddSubPassInput(1, renderPass.GetAttachmentImage(positionAttach));

		CreateDescriptors(system, { &descriptor, &readDescriptor, &glassDescriptor }, "Drawing");
		
		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		Shader& gbufferShader = pipeline.LoadShaderDiffNames(system, "spScene", "GBufferGlass");
		gbufferShader.SetSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, nearPlane);
		gbufferShader.SetSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 1, farPlane);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "FirstPass");

		pipelineRead.SetSubpass(1);
		pipelineRead.LoadShader(system, "Composition");
		CreatePipeline(system, renderPass, pipelineRead, readDescriptor, workingExtent, "SecondPass");

		glassGraphicsPipeline.SetSubpass(2);
		glassGraphicsPipeline.SetupVertexDescription(Attribs::PosTex);
		glassGraphicsPipeline.SetCullMode(VK_CULL_MODE_NONE);	// Glass is facing different ways
		glassGraphicsPipeline.SetDepthWriteEnabled(false);
		glassGraphicsPipeline.EnableBlending(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
		Shader &glassShader = glassGraphicsPipeline.LoadShader(system, "glass");
		glassShader.SetSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, nearPlane);
		glassShader.SetSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 1, farPlane);
		CreatePipeline(system, renderPass, glassGraphicsPipeline, glassDescriptor, workingExtent, "ThirdPass");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);

		vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
		pipelineRead.Bind(commandBuffer, readDescriptor.GetDescriptorSet());
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);

		vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
		glassGraphicsPipeline.Bind(commandBuffer, glassDescriptor.GetDescriptorSet());
		glassModel.Draw(commandBuffer);
	}

private:
	Descriptor descriptor, readDescriptor, glassDescriptor;
	Pipeline pipeline, pipelineRead, glassGraphicsPipeline;
	Model model, glassModel;
	Texture texture;
	uint32_t positionAttach, normalAttach, albedoAttach;
};

DECLARE_APP(SubPassesGBufferGlass)
