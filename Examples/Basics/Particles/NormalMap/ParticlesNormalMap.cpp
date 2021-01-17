#include <VulkanPlayground\Includes.h>
#include <random>

class ParticlesNormalMapApp : public VulkanApplication3DLight
{
	struct VertInfo
	{
		glm::vec3 lightPos;
		float pad1;
		glm::vec3 viewPos;
		int normalMapping;
	};

	void ResetScene() override
	{
		CalcPositionMatrix({ 0.0f, 0.0f, 0.0f }, { -0.7f, -8.0f, 0 }, 0, 30, model.GetModelSize());
		SetupLighting({ 0.0f, -6.0f, 0.0f }, 0.2f, 0.5f, 0.1f, 32.0f, model.GetModelSize());
		normalMapping = true;
		showNormals = 0;
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.CorrectDodgyModelOnLoad();	// Tidy up model data
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "fireplace.obj"), Attribs::PosNormTexTanBitan);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 1, vertInfo, "vertInfo");
		descriptor.AddUniformBuffer(system, 2, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);
		descriptor.AddTexture(system, 3, texture, VulkanPlayground::GetModelFile("Basics", "fireplace_colormap_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		descriptor.AddTexture(system, 4, normalMap, VulkanPlayground::GetModelFile("Basics", "fireplace_normalmap_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		CreateDescriptor(system, descriptor, "Draw");

		pipeline.SetupVertexDescription(Attribs::PosNormTexTanBitan);
		Shader &shader = pipeline.LoadShader(system, "NormalMapTest");
		shader.SetSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, showNormals);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Main");
	};

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('N'))
			normalMapping = !normalMapping;

		if (eventData.KeyPressed('T'))
		{
			showNormals = !showNormals;
			RecreateObjects();
		}
	}

	void UpdateScene(VulkanSystem& system, float /*frameTime*/) override
	{
		vertInfo().lightPos = lightUBO().lightPos;
		vertInfo().viewPos = lightUBO().viewPos;
		vertInfo().normalMapping = normalMapping;
		vertInfo.CopyToDevice(system);
	}

private:
	Descriptor descriptor;
	Pipeline pipeline;
	Model model;
	Texture texture, normalMap;
	UBO<VertInfo> vertInfo;
	bool normalMapping;
	int showNormals;
};

DECLARE_APP(ParticlesNormalMap)
