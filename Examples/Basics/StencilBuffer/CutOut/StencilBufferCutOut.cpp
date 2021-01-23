#include <VulkanPlayground\Includes.h>

class StencilBufferCutOutApp : public VulkanApplication3D
{
#include "..\..\models\quad.vertices"

	bool UseStencilBuffer() override { return true; }

	void ResetScene() override
	{
		CalcPositionMatrix({ 0, 180.0f, 0.0f }, { -3.8f, -0.5f, 0 }, 0, 0, model.GetModelSize());
		stencilOrCutout = true;
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "venus.fbx"), Attribs::Pos);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");

		quadDescriptor.AddUniformBuffer(system, 0, quadUBO, "Quad");
		quadDescriptor.AddTexture(system, 1, quadTexture, VulkanPlayground::GetModelFile("Basics", "darkmetal_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		CreateDescriptors(system, { &descriptor, &quadDescriptor }, "Descriptors");

		pipeline.EnableStencilTest(VK_COMPARE_OP_ALWAYS, VK_STENCIL_OP_REPLACE);
		pipeline.SetDepthTestEnabled(false);	// Don't care about depth values
		pipeline.SetupVertexDescription(Attribs::Pos);
		pipeline.LoadShaderDiffNames(system, "Stencil");	// Only a vertex shader to write stencil values
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Stencil");

		quadPipeline.EnableStencilTest(stencilOrCutout ? VK_COMPARE_OP_NOT_EQUAL : VK_COMPARE_OP_EQUAL, VK_STENCIL_OP_KEEP);
		quadPipeline.SetupVertexDescription(Attribs::PosTex);
		quadPipeline.SetCullMode(VK_CULL_MODE_NONE);	// So both sides of quad drawn
		quadPipeline.LoadShader(system, "quad");
		CreatePipeline(system, renderPass, quadPipeline, quadDescriptor, workingExtent, "Quad");

		system.CreateGpuBuffer(system, vertexBuffer, quadVerticesPT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Quad");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);

		quadPipeline.Bind(commandBuffer, quadDescriptor);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.GetBuffer(), VulkanPlayground::zeroOffset);
		vkCmdDraw(commandBuffer, Attribs::NumVertices(quadVerticesPT, Attribs::PosTex), 1, 0, 0);
	}

	void UpdateScene(VulkanSystem& system, float /*frameTime*/) override
	{
		quadUBO() = mvpUBO();
		quadUBO().model = glm::scale(quadUBO().model, glm::vec3(20.0f, 30.0f, 1.0f));
		quadUBO().model = glm::translate(quadUBO().model, glm::vec3(0.0f, -0.42f, 0.0f));
		quadUBO.CopyToDevice(system);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			stencilOrCutout = !stencilOrCutout;
			RecreateObjects();
		}
	}

private:
	Descriptor descriptor, quadDescriptor;
	Pipeline pipeline, quadPipeline;
	Model model;
	UBO<MVP> quadUBO;
	Texture quadTexture;
	Buffer vertexBuffer;
	bool stencilOrCutout = true;
};

DECLARE_APP(StencilBufferCutOut)
