#include <VulkanPlayground\Includes.h>

class StencilBufferObscuredApp : public VulkanApplication3DSimpleLight
{
	#include "..\..\models\quad.vertices"
	
	bool UseStencilBuffer() override { return useStencil; }

	void ResetScene() override
	{
		CalcPositionMatrix({ 0, 180.0f, 0.0f }, { -3.8f, -0.8f, 0 }, 0, 10, model.GetModelSize());
		SetupLighting({ 0.0f, 1.0f, 2.0f }, { 0.1f, 0.25f, 0.5f, 0.9f }, { 0.1f, 0.2f, 0.4f, 0.6f }, 1.3f, model.GetModelSize());
		useStencil = true;
		showHighlight = true;
		RecreateObjects();
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "venus.fbx"), Attribs::PosNorm);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 2, lightToonUBO, "Toon Light", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Draw");

		if (useStencil)
			pipeline.EnableStencilTest(VK_COMPARE_OP_ALWAYS, VK_STENCIL_OP_REPLACE);
		pipeline.SetupVertexDescription(Attribs::PosNorm);
		pipeline.LoadShader(system, "SceneN");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Model");

		if (useStencil)
		{
			highlightPipeline.EnableStencilTest(VK_COMPARE_OP_NOT_EQUAL, VK_STENCIL_OP_KEEP);
			highlightPipeline.SetDepthTestEnabled(false);	// Show highlight on top of everything
		}
		else
		{
			highlightPipeline.SetDepthTestEnabled(true);
			highlightPipeline.SetDepthWriteEnabled(false);	// Don't update depth values for highlight draw
		}
		highlightPipeline.SetupVertexDescription(Attribs::PosNorm);
		highlightPipeline.LoadShader(system, "Outline");
		CreatePipeline(system, renderPass, highlightPipeline, descriptor, workingExtent, "Highlight");

		quadDescriptor.AddUniformBuffer(system, 0, quadUBO, "Quad");
		quadDescriptor.AddTexture(system, 1, quadTexture, VulkanPlayground::GetModelFile("Basics", "darkmetal_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		CreateDescriptor(system, quadDescriptor, "Quad");

		quadPipeline.SetupVertexDescription(Attribs::PosTex);
		quadPipeline.SetCullMode(VK_CULL_MODE_NONE);	// So both sides of quad drawn
		quadPipeline.LoadShader(system, "quad");
		CreatePipeline(system, renderPass, quadPipeline, quadDescriptor, workingExtent, "Quad");

		system.CreateGpuBuffer(system, vertexBuffer, quadVerticesPT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Quad");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		quadPipeline.Bind(commandBuffer, quadDescriptor.GetDescriptorSet());
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.GetBuffer(), VulkanPlayground::zeroOffset);
		vkCmdDraw(commandBuffer, Attribs::NumVertices(quadVerticesPT, Attribs::PosTex), 1, 0, 0);

		if (useStencil)
		{
			pipeline.Bind(commandBuffer, descriptor);
			model.Draw(commandBuffer);
		}
		if (showHighlight)
		{
			highlightPipeline.Bind(commandBuffer, descriptor);
			model.Draw(commandBuffer);
		}
		if (!useStencil)
		{
			pipeline.Bind(commandBuffer, descriptor);
			model.Draw(commandBuffer);
		}
	}

	void UpdateScene(VulkanSystem& system, float /*frameTime*/) override
	{
		quadUBO() = mvpUBO();
		quadUBO().model = glm::scale(quadUBO().model, glm::vec3(30.0f, 10.0f, 1.0f));
		quadUBO().model = glm::translate(quadUBO().model, glm::vec3(0.0f, -1.5f, 10.0f));
		quadUBO.CopyToDevice(system);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			useStencil = !useStencil;
			RecreateObjects();
		}
		if (eventData.KeyPressed('H'))
		{
			showHighlight = !showHighlight;
			RedrawScene();
		}
	}

private:
	Descriptor descriptor, quadDescriptor;
	Pipeline pipeline, quadPipeline, highlightPipeline;
	Model model;

	UBO<MVP> quadUBO;
	Texture quadTexture;
	Buffer vertexBuffer;

	bool useStencil = true;
	bool showHighlight = true;
};

DECLARE_APP(StencilBufferObscured)
