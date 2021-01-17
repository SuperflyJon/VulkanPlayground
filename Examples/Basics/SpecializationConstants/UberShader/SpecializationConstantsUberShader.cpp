#include <VulkanPlayground\Includes.h>

class SpecializationConstantsUberShaderApp : public VulkanApplication3DLight
{
	struct SpecializationData
	{
		uint32_t lightingModel;	// Sets the lighting model used in the fragment "uber" shader
		float toonDesaturationFactor;	// Sample 2nd parameter
	} specializationData;

	void ResetScene() override
	{
		UseEyeLightSpace();
		CalcPositionMatrix({ 0, 0, 0 }, { -1.5, -5.0, 0 }, 0, 40, model.GetModelSize());
		SetupLighting({ 0.6f, 0, -0.4f }, 0.5f, 1.0f, 0.4f, 30.0f, model.GetModelSize());
		VulkanApplication3DSimpleLight::SetupLighting(lightToonUBO, { 0.6f, 0, -0.4f }, { 0.25f, 0.5f, 0.9f, 0.98f }, { 0.13f, 0.4f, 0.66f, 0.75f }, 2.0f, model.GetModelSize());
		RecreateObjects();
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		VulkanPlayground::SetupProjectionMatrix(mvpUBO().projection, AspectRatio() / 3);

		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "color_teapot_spheres.dae"), Attribs::PosNormColTex);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 1, lightUBO, "Light", VK_SHADER_STAGE_FRAGMENT_BIT);
		descriptor.AddTexture(system, 2, texture, VulkanPlayground::GetModelFile("Basics", "metalplate_nomips_rgba.ktx"));
		descriptor.AddUniformBuffer(system, 3, lightToonUBO, "ToonLight", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Drawing");

		Shader &uberShader1 = pipeline1.LoadShader(system, "Uber");
		uberShader1.SetSpecializationConstantsFromStruct(VK_SHADER_STAGE_FRAGMENT_BIT, &specializationData, { sizeof(specializationData.lightingModel), sizeof(specializationData.toonDesaturationFactor) });
		pipeline1.SetPipelineFlags(VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT);
		pipeline1.SetCullMode(VK_CULL_MODE_NONE);	// Hide issues with teapot model
		pipeline1.EnableDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
		pipeline1.SetupVertexDescription(Attribs::PosNormColTex);
		specializationData.lightingModel = 1;
		CreatePipeline(system, renderPass, pipeline1, descriptor, workingExtent, "Scene1");

		pipeline2.DerivePipeline(pipeline1);
		specializationData.lightingModel = 2;
		specializationData.toonDesaturationFactor = 4;
		Shader& uberShader2 = pipeline2.LoadShader(system, "Uber");
		uberShader2.SetSpecializationConstantsFromStruct(VK_SHADER_STAGE_FRAGMENT_BIT, &specializationData, { sizeof(specializationData.lightingModel), sizeof(specializationData.toonDesaturationFactor) });
		CreatePipeline(system, renderPass, pipeline2, descriptor, workingExtent, "Scene2");

		pipeline3.DerivePipeline(pipeline1);
		specializationData.lightingModel = 3;
		Shader& uberShader3 = pipeline3.LoadShader(system, "Uber");
		uberShader3.SetSpecializationConstantsFromStruct(VK_SHADER_STAGE_FRAGMENT_BIT, &specializationData, { sizeof(specializationData.lightingModel), sizeof(specializationData.toonDesaturationFactor) });
		CreatePipeline(system, renderPass, pipeline3, descriptor, workingExtent, "Scene3");
	}

	void UpdateScene(VulkanSystem& system, float /*frameTime*/) override
	{
		// Convert lightPos to eye space
		auto savedLightPos = lightToonUBO().lightPos;
		lightToonUBO().lightPos = glm::vec3(mvpUBO().view * glm::vec4(lightToonUBO().lightPos, 1.0f));
		lightToonUBO.CopyToDevice(system);
		lightToonUBO().lightPos = savedLightPos;
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		float widthThird = (float)GetWindowWidth() / 3.0f;
		float height = (float)GetWindowHeight();

		VulkanPlayground::SetViewport(commandBuffer, 0, 0, widthThird, height);
		pipeline1.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);

		VulkanPlayground::SetViewport(commandBuffer, widthThird, 0, widthThird, height);
		pipeline2.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);

		VulkanPlayground::SetViewport(commandBuffer, 2 * widthThird, 0, widthThird, height);
		pipeline3.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);
	}

private:
	Texture texture;
	Pipeline pipeline1, pipeline2, pipeline3;
	Descriptor descriptor;
	Model model;
	UBO<UBO_lightToon> lightToonUBO;	// For scene 2
};

DECLARE_APP(SpecializationConstantsUberShader)
