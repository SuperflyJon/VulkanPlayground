#include <VulkanPlayground\Includes.h>

class OffscreenReflectApp : public VulkanApplication3DLight
{
	 std::vector<float> quadVertices = {
		-10.0f, 0.0f, -10.0f,  0.0f, 1.0f,   10.0f, 0.0f,  10.0f,  1.0f, 0.0f,   10.0f, 0.0f, -10.0f,  1.0f, 1.0f,
		 10.0f, 0.0f, 10.0f,   1.0f, 0.0f,  -10.0f, 0.0f, -10.0f,  0.0f, 1.0f,  -10.0f, 0.0f,  10.0f,  0.0f, 0.0f };

	const float floorOffset = (10.0f / 2.0f) * .95f;
	VkExtent2D offscreenExtent{ 512, 512 };

	void ResetScene() override
	{
		CalcPositionMatrix({ 0.0f, 180.0f, 0.0f }, { -1.3f, -1.3f, 0 }, 0, 30, model.GetModelSize());
		SetupLighting({ -1.7f, 0.5f, 1.0f }, 0.2f, 0.4f, 0.7f, 32.0f, model.GetModelSize());
		showBlur = true;
	}

	void SetupRenderPasses(RenderPasses& renderPasses, VulkanSystem& system, VkFormat imageFormat) override
	{
		VulkanApplication::SetupRenderPasses(renderPasses, system, imageFormat);	// Create main renderpasses as normal

		// Offscreen rendering
		OffscreenRenderPass& offscreenRenderPass = renderPasses.CreateOffscreenRenderPass(offscreenExtent);
		offscreenRenderPass.AddIntermidiateColourAttachment(VK_FORMAT_R8G8B8A8_UNORM, "Offscreen", VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);	// Need to store the result so can be read in main renderpass
		offscreenRenderPass.AddDepthAttachment(GetDepthFormat(system));
	}

	void SetupObjects(VulkanSystem& system, RenderPasses& renderPasses, VkExtent2D workingExtent) override
	{
		model.SetTranslation({ 0, floorOffset, 0 });	// Move dragon up so it's on the y axis
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "chinesedragon.dae"), Attribs::PosNormCol);
		system.CreateGpuBuffer(system, planeBuffer, quadVertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Vertices");

		offscreenDescriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		offscreenDescriptor.AddUniformBuffer(system, 2, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);

		offscreenPipeline.SetupVertexDescription(Attribs::PosNormCol);
		offscreenPipeline.LoadShader(system, "offScene");
		offscreenPipeline.SetCullMode(VK_CULL_MODE_FRONT_BIT);	// As model is upside down

		modelDescriptor.AddUniformBuffer(system, 0, uboModel, "MVP");
		modelDescriptor.AddUniformBuffer(system, 2, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);
		modelPipeline.SetupVertexDescription(Attribs::PosNormCol);
		modelPipeline.LoadShader(system, "offScene");

		reflectDescriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		reflectDescriptor.AddTexture(system, 1, planeTexture, VulkanPlayground::GetModelFile("Basics", "darkmetal_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		reflectDescriptor.AddTexture(2, renderPasses.GetOffscreenRenderPass()->GetBuffer());
		CreateDescriptors(system, { &offscreenDescriptor, &modelDescriptor, &reflectDescriptor }, "Descriptors");

		reflectPipeline.SetupVertexDescription(Attribs::PosTex);
		reflectPipeline.AddPushConstant(system, sizeof(showBlur), VK_SHADER_STAGE_FRAGMENT_BIT);
		reflectPipeline.LoadShader(system, "Reflect");

		CreatePipeline(system, *renderPasses.GetOffscreenRenderPass(), offscreenPipeline, offscreenDescriptor, offscreenExtent, "Offscreen");
		CreatePipeline(system, renderPasses.GetScreenRenderPass(), modelPipeline, modelDescriptor, workingExtent, "Model");
		CreatePipeline(system, renderPasses.GetScreenRenderPass(), reflectPipeline, reflectDescriptor, workingExtent, "Plane");
	};

	void CreateCommandBuffers(VulkanSystem& system, RenderPasses& renderPasses) override
	{
		VulkanApplication::CreateCommandBuffers(system, renderPasses);	// Create main command buffers as normal

		renderPasses.GetOffscreenRenderPass()->GetDisplayBuffers().CreateCmdBuffers(system, *renderPasses.GetOffscreenRenderPass(),
			[this, &system](VkCommandBuffer commandBuffer)
			{
				system.DebugStartRegion(commandBuffer, "Offscreen", { 1.0f, 0.0f, 1.0f });
				offscreenPipeline.Bind(commandBuffer, offscreenDescriptor);
				model.Draw(commandBuffer);
				system.DebugEndRegion(commandBuffer);
			}, "OffscreenCmds");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		modelPipeline.Bind(commandBuffer, modelDescriptor);
		model.Draw(commandBuffer);

		reflectPipeline.PushConstant(commandBuffer, &showBlur);
		reflectPipeline.Bind(commandBuffer, reflectDescriptor);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &planeBuffer.GetBuffer(), VulkanPlayground::zeroOffset);
		vkCmdDraw(commandBuffer, Attribs::NumVertices(quadVertices, Attribs::PosTex), 1, 0, 0);
	}

	void UpdateScene(VulkanSystem& system, float /*frameTime*/) override
	{
		uboModel() = mvpUBO();
		uboModel.CopyToDevice(system);

		mvpUBO().model = glm::scale(mvpUBO().model, glm::vec3(1.0f, -1.0f, 1.0f));	// Flip upside down
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			showBlur = !showBlur;
			reflectPipeline.Recreate();
		}
	}

private:
	Model model;
	Buffer planeBuffer;
	int showBlur;
	Descriptor modelDescriptor, offscreenDescriptor, reflectDescriptor;
	Pipeline modelPipeline, offscreenPipeline, reflectPipeline;
	UBO<MVP> uboModel;
	Texture planeTexture;
};

DECLARE_APP(OffscreenReflect)
