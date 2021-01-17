#include <VulkanPlayground\Includes.h>
#include <glm\gtx\rotate_vector.hpp>

class CubeMapModelsApp : public VulkanApplication3DLight
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
			models[model].LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", modelNames[model]), Attribs::PosNormCol);
		}

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 2, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		pipeline.LoadShader(system, "Models");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Models");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
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
	}

	void UpdateScene(VulkanSystem& /*system*/, float frameTime) override
	{
		if (rotateCamera)
			rotateCameraAmount += frameTime / 2;

		glm::vec3 viewPos = glm::rotateY(GetCameraPos().GetPosition(), rotateCameraAmount);
		mvpUBO().view = VulkanPlayground::CalcViewMatrix(viewPos, -rotateCameraAmount, 0);
	}

private:
	std::vector<Model> models;
	Pipeline pipeline;
	Descriptor descriptor;

	size_t curModel = 0;
	bool rotateCamera;
	float rotateCameraAmount;
};

DECLARE_APP(CubeMapModels)
