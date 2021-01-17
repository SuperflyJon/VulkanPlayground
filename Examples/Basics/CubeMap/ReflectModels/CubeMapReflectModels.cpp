#include <VulkanPlayground\Includes.h>
#include <glm\gtx\rotate_vector.hpp>

class CubeMapReflectModelsApp : public VulkanApplication3DLight
{
	void ResetScene() override
	{
		rotateCamera = true;
		rotateCameraAmount = 0;
		SetupMVP();
	}
	void SetupMVP()
	{
		CenterModel(models[curModel]);
		SetupLighting({ 0, -45, 8 }, 0.2f, 0.8f, 0.20f, 50.0f, models[curModel].GetModelSize());
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		const std::vector<std::string> modelNames{ "sphere.obj", "teapot.dae", "torusknot.obj", "venus.fbx" };
		models.resize(modelNames.size());
		for (int model = 0; model < models.size(); model++)
		{
			models[model].LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", modelNames[model]), Attribs::PosNorm);
		}

		cubeTexture.LoadCubeMap(system, VulkanPlayground::GetModelFile("Basics", "cubemap_yokohama_bc3.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 2, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);
		descriptor.AddTexture(1, cubeTexture);
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosNorm);
		pipeline.LoadShader(system, "ReflectModels");
		pipeline.AddPushConstant(system, sizeof(showReflection), VK_SHADER_STAGE_FRAGMENT_BIT);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "ReflectModels");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.PushConstant(commandBuffer, &showReflection);

		pipeline.Bind(commandBuffer, descriptor);
		models[curModel].Draw(commandBuffer);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('M'))
		{
			curModel = ++curModel % models.size();
			SetupMVP();
			RedrawScene();
		}
		if (eventData.KeyPressed(' '))
			rotateCamera = !rotateCamera;
		
		if (eventData.KeyPressed('T'))
		{
			showReflection = !showReflection;
			RedrawScene();
		}
	}

	void UpdateScene(VulkanSystem& , float frameTime) override
	{
		if (rotateCamera)
			rotateCameraAmount += frameTime / 2;

		glm::vec3 viewPos = glm::rotateY(GetCameraPos().GetPosition(), rotateCameraAmount);
		mvpUBO().view = VulkanPlayground::CalcViewMatrix(viewPos, -rotateCameraAmount, 0);
	}

private:
	int showReflection = 1;	// Push constant variable
	std::vector<Model> models;
	Pipeline pipeline;
	Descriptor descriptor;
	Texture cubeTexture;

	size_t curModel = 0;
	bool rotateCamera;
	float rotateCameraAmount;
};

DECLARE_APP(CubeMapReflectModels)
