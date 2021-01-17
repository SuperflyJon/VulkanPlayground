#include <VulkanPlayground\Includes.h>

class SpecializationConstantsUberShaderSingleApp : public VulkanApplication3DLight
{
	struct SpecializationData
	{
		uint32_t lightingModel;	// Sets the lighting model used in the fragment "uber" shader
		float toonDesaturationFactor;	// Sample 2nd parameter
	} specializationData;

	void ResetScene() override
	{
		UseEyeLightSpace();
		CalcPositionMatrix({ 0, 0, 0 }, { -0.9, -3.0, 0 }, 0, 40, model.GetModelSize());
		SetupLighting({ 0.6f, 0, -0.4f }, 0.5f, 1.0f, 0.4f, 30.0f, model.GetModelSize());
		VulkanApplication3DSimpleLight::SetupLighting(lightToonUBO, { 0.6f, 0, -0.4f }, { 0.25f, 0.5f, 0.9f, 0.98f }, { 0.13f, 0.4f, 0.66f, 0.75f }, 2.0f, model.GetModelSize());

		// Reset to first
		specializationData.lightingModel = 1;
		pipeline1.Recreate();	// Need to recreate the pipeline to use the new specialization constant value

	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "color_teapot_spheres.dae"), Attribs::PosNormColTex);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 1, lightUBO, "Light", VK_SHADER_STAGE_FRAGMENT_BIT);
		descriptor.AddTexture(system, 2, texture, VulkanPlayground::GetModelFile("Basics", "metalplate_nomips_rgba.ktx"));
		descriptor.AddUniformBuffer(system, 3, lightToonUBO, "ToonLight", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline1.SetCullMode(VK_CULL_MODE_NONE);	// Hide issues with teapot model
		pipeline1.SetupVertexDescription(Attribs::PosNormColTex);
		Shader& uberShader = pipeline1.LoadShader(system, "Uber");
		uberShader.SetSpecializationConstantsFromStruct(VK_SHADER_STAGE_FRAGMENT_BIT, &specializationData, { sizeof(specializationData.lightingModel), sizeof(specializationData.toonDesaturationFactor) });
		specializationData.toonDesaturationFactor = 4;
		CreatePipeline(system, renderPass, pipeline1, descriptor, workingExtent, "Scene1");
	}

	void UpdateScene(VulkanSystem& system, float /*frameTime*/) override
	{
		// Convert lightPos to eye space
		auto savedLightPos = lightToonUBO().lightPos;
		lightToonUBO().lightPos = glm::vec3(mvpUBO().view * glm::vec4(lightToonUBO().lightPos, 1.0f));
		lightToonUBO.CopyToDevice(system);
		lightToonUBO().lightPos = savedLightPos;
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			specializationData.lightingModel++;
			if (specializationData.lightingModel == 4)
				specializationData.lightingModel = 1;
			pipeline1.Recreate();	// Need to recreate the pipeline to use the new specialization constant value
		}
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline1.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);
	}

private:
	Texture texture;
	Pipeline pipeline1;
	Descriptor descriptor;
	Model model;
	UBO<UBO_lightToon> lightToonUBO;	// For scene 2
};

DECLARE_APP(SpecializationConstantsUberShaderSingle)
