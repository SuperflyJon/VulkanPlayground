#include <VulkanPlayground\Includes.h>

class ModelLitTestMeshApp : public VulkanApplication3DLight
{
	void ResetScene() override
	{
		CalcPositionMatrix({ 0, 20, 0 }, { -1.0f, -0.4f, 0 }, 0, 25, model.GetModelSize());
		SetupLighting({ 0, -45, 8 }, 0.5f, 0.5f, 0.75f, 16.0f, model.GetModelSize());
		SetLineMode(false);
	}

	void SetLineMode(bool val)
	{
		lineMode = val;
		pipeline.SetPolygonMode(lineMode ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL);
		pipeline.Recreate();
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "voyager.dae"), Attribs::PosNormTex);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddTexture(system, 2, texture, VulkanPlayground::GetModelFile("Basics", "voyager_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		descriptor.AddUniformBuffer(system, 1, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosNormTex);
		pipeline.LoadShader(system, "LitTestMesh");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "TestModel");
	};

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('L'))
			SetLineMode(!lineMode);
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Model model;
	Texture texture;
	bool lineMode;
};

DECLARE_APP(ModelLitTestMesh)
