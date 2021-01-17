#include <VulkanPlayground\Includes.h>

class OffscreenSceneApp : public VulkanApplication3DLight
{
	std::vector<float> quadVertices = {
	   -10.0f, 0.0f, -10.0f,  0.0f, 1.0f,   10.0f, 0.0f,  10.0f,  1.0f, 0.0f,   10.0f, 0.0f, -10.0f,  1.0f, 1.0f,
		10.0f, 0.0f, 10.0f,   1.0f, 0.0f,  -10.0f, 0.0f, -10.0f,  0.0f, 1.0f,  -10.0f, 0.0f,  10.0f,  0.0f, 0.0f };

	void ResetScene() override
	{
		CalcPositionMatrix({ 0.0f, 180.0f, 0.0f }, { -1.2f, -1.4f, 0 }, 0, 30, model.GetModelSize());
		SetupLighting({ -1.7f, 0.5f, 1.0f }, 0.2f, 0.4f, 0.7f, 32.0f, model.GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.SetTranslation({ 0, (10.0f / 2.0f) * .95f, 0 });	// Move dragon up so it's on the y axis
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "chinesedragon.dae"), Attribs::PosNormCol);
		system.CreateGpuBuffer(system, planeBuffer, quadVertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Vertices");

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 2, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Main");

		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		pipeline.LoadShader(system, "offScene");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Main");

		planeDescriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		planeDescriptor.AddTexture(system, 1, texture, VulkanPlayground::GetModelFile("Basics", "darkmetal_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		CreateDescriptor(system, planeDescriptor, "Plane");

		planePipeline.SetupVertexDescription(Attribs::PosTex);
		planePipeline.LoadShader(system, "quad");
		CreatePipeline(system, renderPass, planePipeline, planeDescriptor, workingExtent, "Plane");
	};

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);

		planePipeline.Bind(commandBuffer, planeDescriptor.GetDescriptorSet());
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &planeBuffer.GetBuffer(), VulkanPlayground::zeroOffset);
		vkCmdDraw(commandBuffer, Attribs::NumVertices(quadVertices, Attribs::PosTex), 1, 0, 0);
	}

private:
	Model model;
	Buffer planeBuffer;
	Descriptor descriptor, planeDescriptor;
	Pipeline pipeline, planePipeline;
	Texture texture;
};

DECLARE_APP(OffscreenScene)
