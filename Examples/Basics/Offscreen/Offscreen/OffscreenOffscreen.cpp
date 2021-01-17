#include <VulkanPlayground\Includes.h>

class OffscreenOffscreenApp : public VulkanApplication3DLight
{
	const float floorOffset = (10.0f / 2.0f) * .95f;
	VkExtent2D offscreenExtent{ 512, 512 };

	void ResetScene() override
	{
		CalcPositionMatrix({ 0.0f, 180.0f, 0.0f }, { -1.5f, 0, 0 }, 0, 0, model.GetModelSize());
		SetupLighting({ -1.7f, 0.5f, 1.0f }, 0.2f, 0.4f, 0.7f, 32.0f, model.GetModelSize());
		flip = true;
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

		descriptorOffscreen.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptorOffscreen.AddUniformBuffer(system, 2, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptorOffscreen, "Offscreen");

		pipelineOffscreen.SetupVertexDescription(Attribs::PosNormCol);
		pipelineOffscreen.LoadShader(system, "offScene");
		pipelineOffscreen.SetCullMode(VK_CULL_MODE_NONE);	// So model draws correctly both ways up
		CreatePipeline(system, *renderPasses.GetOffscreenRenderPass(), pipelineOffscreen, descriptorOffscreen, offscreenExtent, "Offscreen");

		descriptor.AddTexture(1, renderPasses.GetOffscreenRenderPass()->GetBuffer());
		CreateDescriptor(system, descriptor, "Main");

		pipeline.LoadShader(system, "Test");
		CreatePipeline(system, renderPasses.GetScreenRenderPass(), pipeline, descriptor, workingExtent, "Main");
	};

	void CreateCommandBuffers(VulkanSystem& system, RenderPasses& renderPasses) override
	{
		VulkanApplication::CreateCommandBuffers(system, renderPasses);	// Create main command buffers as normal

		renderPasses.GetOffscreenRenderPass()->GetDisplayBuffers().CreateCmdBuffers(system, *renderPasses.GetOffscreenRenderPass(),
			[this, &system](VkCommandBuffer commandBuffer)
			{
				system.DebugStartRegion(commandBuffer, "Offscreen", { 1.0f, 0.0f, 1.0f });
				pipelineOffscreen.Bind(commandBuffer, descriptorOffscreen);
				model.Draw(commandBuffer);
				system.DebugEndRegion(commandBuffer);
			}, "OffscreenCmds");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	}

	void UpdateScene(VulkanSystem& /*system*/, float /*frameTime*/) override
	{
		if (flip)
			mvpUBO().model = glm::scale(mvpUBO().model, glm::vec3(1.0f, -1.0f, 1.0f));	// Flip upside down
		mvpUBO().model = glm::translate(mvpUBO().model, glm::vec3(0.0f, floorOffset, 0.0f));
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			flip = !flip;
		}
		if (eventData.KeyPressed('L'))
		{	// Test - show low res offscreen buffer
			if (offscreenExtent.height == 512)
				offscreenExtent = { 128, 128 };
			else
				offscreenExtent = { 512, 512 };
			RecreateObjects();
		}
	}

private:
	Model model;
	Descriptor descriptor, descriptorOffscreen;
	Pipeline pipeline, pipelineOffscreen;
	bool flip;
};

DECLARE_APP(OffscreenOffscreen)
