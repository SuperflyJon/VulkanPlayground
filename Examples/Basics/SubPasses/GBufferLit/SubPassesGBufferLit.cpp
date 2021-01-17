#include <VulkanPlayground\Includes.h>
#include <random>
#include <glm\gtx\rotate_vector.hpp>

class SubPassesGBufferLitApp : public VulkanApplication3DLight
{
	static constexpr auto NUM_SPOTS = 64;

	struct Spotlight
	{
		glm::vec3 lookatPos;
		int pad2;
		glm::vec3 colour;
		float coneSize;
	};

	struct SpotUBO
	{
		Spotlight spots[NUM_SPOTS];

		int showMainLight;
		int showSpot;
		int hardSpot;
		float spotBrightness;
	};

	void ResetScene() override
	{
		CalcPositionMatrix({ 0.0f, 15.0f, 0.0f }, { -0.3, -0.1, -0.05 }, 0, 0, model.GetModelSize());
		SetupLighting({ 1.0f, -0.8f, -0.8f }, 0.3f, 0.2f, 0.2f, 32.0f, model.GetModelSize());

		spotUBO().showMainLight = true;
		spotUBO().showSpot = true;
		spotUBO().hardSpot = false;
		spotUBO().spotBrightness = 0.5;
		CalculateSpotPositions();

		rotate = true;
		rotateAmount = 0;
	}
	void CalculateSpotPositions()
	{
		std::uniform_real_distribution<float> rndDist(0, 1);
		for (uint32_t index = 0; index < NUM_SPOTS; index++)
		{
			auto& spot = spotUBO().spots[index];
			// Set random position (x, y, z) and speed (w) in spotPositions array
			spotPositions[index] = glm::vec4(rndDist(rndEngine) * 10.0f - 5.0f, rndDist(rndEngine) * -3.0f, rndDist(rndEngine) * 20.0f - 10.0f, rndDist(rndEngine) * 1.0f + 1.0f);
			spot.lookatPos = Rotate(spotPositions[index], rotateAmount * spotPositions[index][3], index % 3);
			// Set up random colours and spot light size
			spot.colour = { rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine) };
			spot.coneSize = cos(glm::radians(rndDist(rndEngine) * 1.5f + 0.5f));
		}
	}

	void SetupRenderPasses(RenderPasses& renderPasses, VulkanSystem& system, VkFormat imageFormat) override
	{
		RenderPass& renderPass = renderPasses.NewRenderPass();
		// First sub-pass, render off screen
		positionAttach = renderPass.AddIntermidiateColourAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, "Position");
		albedoAttach = renderPass.AddIntermidiateColourAttachment(VK_FORMAT_R8G8B8A8_UNORM, "Albedo");
		normalAttach = renderPass.AddIntermidiateColourAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, "Normal");
		depthAttach = renderPass.AddDepthAttachment(GetDepthFormat(system));

		// Second sub-pass, display gbuffer components on screen
		renderPass.AddNewSubPass();
		uint32_t colourAttach = renderPass.AddColourAttachment(imageFormat, "MainBuffer", VK_ATTACHMENT_LOAD_OP_DONT_CARE);
		renderPass.AddInput(positionAttach);
		renderPass.AddInput(albedoAttach);
		renderPass.AddInput(normalAttach);

		// Third sub-pass, draw glass windows
		renderPass.AddNewSubPass();
		renderPass.AddExistingColourAttachment(colourAttach);
		renderPass.AddInput(depthAttach);
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		const float nearPlane = 0.1f;
		const float farPlane = 256.0f;
	
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "samplebuilding.dae"), Attribs::PosNormCol);
		glassModel.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "samplebuilding_glass.dae"), Attribs::PosTex);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");

		readDescriptor.AddUniformBuffer(system, 3, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);
		readDescriptor.AddUniformBuffer(system, 4, spotUBO, "SpotLights", VK_SHADER_STAGE_FRAGMENT_BIT);
		readDescriptor.AddSubPassInput(0, renderPass.GetAttachmentImage(positionAttach));
		readDescriptor.AddSubPassInput(1, renderPass.GetAttachmentImage(albedoAttach));
		readDescriptor.AddSubPassInput(2, renderPass.GetAttachmentImage(normalAttach));

		glassDescriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		glassDescriptor.AddTexture(system, 2, texture, VulkanPlayground::GetModelFile("Basics", "coloured_glass_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		glassDescriptor.AddSubPassInput(1, renderPass.GetAttachmentImage(depthAttach));

		CreateDescriptors(system, { &descriptor, &readDescriptor, &glassDescriptor }, "Drawing");
		
		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		Shader &gbufferShader = pipeline.LoadShaderDiffNames(system, "spScene", "GBufferLit");
		gbufferShader.SetSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, nearPlane);
		gbufferShader.SetSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 1, farPlane);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "FirstPass");

		pipelineRead.SetSubpass(1);
		Shader &readShader = pipelineRead.LoadShaderDiffNames(system, "Composition", "CompositionSpot");
		readShader.SetSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, NUM_SPOTS);
		CreatePipeline(system, renderPass, pipelineRead, readDescriptor, workingExtent, "SecondPass");

		glassGraphicsPipeline.SetSubpass(2);
		glassGraphicsPipeline.SetupVertexDescription(Attribs::PosTex);
		glassGraphicsPipeline.SetCullMode(VK_CULL_MODE_NONE);	// Glass is facing different ways
		glassGraphicsPipeline.SetDepthWriteEnabled(false);
		glassGraphicsPipeline.EnableBlending(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
		Shader &glassShader = glassGraphicsPipeline.LoadShaderDiffNames(system, "glass", "glassDepth");
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

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('L'))
			spotUBO().showMainLight = !spotUBO().showMainLight;

		if (eventData.KeyPressed('T'))
			spotUBO().showSpot = !spotUBO().showSpot;

		if (eventData.KeyPressed('H'))
			spotUBO().hardSpot = !spotUBO().hardSpot;

		if (eventData.KeyPressed(' '))
			rotate = !rotate;
	}

	glm::vec3 Rotate(glm::vec3 pos, float amount, int axis)
	{
		switch (axis)
		{
		case 0:	return glm::rotateX(pos, amount);
		case 1:	return glm::rotateY(pos, amount);
		case 2:	return glm::rotateZ(pos, amount);
		default: throw std::runtime_error("Invalid argument");
		}
	}

	void UpdateScene(VulkanSystem& system, float frameTime) override
	{
		if (rotate)
		{
			rotateAmount += frameTime / 3.0f;
			for (uint32_t index = 0; index < NUM_SPOTS; index++)
				spotUBO().spots[index].lookatPos = Rotate(spotPositions[index], rotateAmount * spotPositions[index][3], index % 3);
		}
		spotUBO.CopyToDevice(system);
	}

private:
	Descriptor descriptor, readDescriptor, glassDescriptor;
	Pipeline pipeline, pipelineRead, glassGraphicsPipeline;
	Model model, glassModel;
	Texture texture;
	UBO<SpotUBO> spotUBO;

	uint32_t positionAttach, normalAttach, albedoAttach, depthAttach;
	int showAlbedo = 0;
	bool rotate;
	float rotateAmount;
	glm::vec4 spotPositions[NUM_SPOTS];
	std::default_random_engine rndEngine; 
	int attachType = 0;
};

DECLARE_APP(SubPassesGBufferLit)
