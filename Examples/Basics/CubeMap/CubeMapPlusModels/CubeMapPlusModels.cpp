#include <VulkanPlayground\Includes.h>
#include <glm\gtx\rotate_vector.hpp>

class CubeMapCubeMapPlusModelsApp : public VulkanApplication3DLight
{
	void ResetScene() override
	{
		rotateCamera = true;
		rotateCameraAmount = 0;
		showModel = true;
		showSkybox = true;		
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
		cubeModel.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "cube.obj"), Attribs::Pos);

		// Models
		models.resize(modelNames.size());
		for (int model = 0; model < models.size(); model++)
		{
			models[model].LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", modelNames[model]), Attribs::PosNorm);
		}

		cubeTexture.LoadCubeMap(system, VulkanPlayground::GetModelFile("Basics", "cubemap_yokohama_bc3.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		descriptorModel.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptorModel.AddUniformBuffer(system, 2, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);
		descriptorModel.AddTexture(1, cubeTexture);
		CreateDescriptor(system, descriptorModel, "Drawing");

		pipelineModel.SetupVertexDescription(Attribs::PosNorm);
		pipelineModel.LoadShader(system, "ReflectModels");
		pipelineModel.AddPushConstant(system, sizeof(showReflection), VK_SHADER_STAGE_FRAGMENT_BIT);
		CreatePipeline(system, renderPass, pipelineModel, descriptorModel, workingExtent, "ReflectModels");

		// Skybox
		descriptorCube.AddUniformBuffer(system, 0, uniformBufferCube, "MVPCube");
		descriptorCube.AddTexture(1, cubeTexture);
		CreateDescriptor(system, descriptorCube, "Box");

		pipelineCube.SetWindingOrder(VK_FRONT_FACE_CLOCKWISE);	// Drawing on inside of cube
		pipelineCube.SetDepthTestOp(VK_COMPARE_OP_LESS_OR_EQUAL);	// Maximum depth values for skybox so have to set test to <= 1 others it won't show up correctly
		pipelineCube.SetupVertexDescription(Attribs::Pos);
		pipelineCube.LoadShader(system, "CubeMap");
		CreatePipeline(system, renderPass, pipelineCube, descriptorCube, workingExtent, "SkyBox");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		if (showModel)
		{
			pipelineModel.PushConstant(commandBuffer, &showReflection);
			pipelineModel.Bind(commandBuffer, descriptorModel.GetDescriptorSet());
			models[curModel].Draw(commandBuffer);
		}
		if (showSkybox)
		{
			pipelineCube.Bind(commandBuffer, descriptorCube.GetDescriptorSet());
			cubeModel.Draw(commandBuffer);
		}
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('M'))
		{
			curModel = ++curModel % models.size();
			SetupMVP();
		}
		if (eventData.KeyPressed(' '))
			rotateCamera = !rotateCamera;
		
		if (eventData.KeyPressed('T'))
			showReflection = !showReflection;

		if (eventData.KeyPressed('N'))
			showModel = !showModel;

		if (eventData.KeyPressed('B'))
			showSkybox = !showSkybox;

		RedrawScene();
	}

	void UpdateScene(VulkanSystem& system, float frameTime) override
	{
		if (rotateCamera)
			rotateCameraAmount += frameTime / 2;

		uniformBufferCube() = mvpUBO();
		uniformBufferCube().model = glm::rotate(uniformBufferCube().model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		glm::vec3 scenePos = glm::rotateY(GetCameraPos().GetPosition(), -rotateCameraAmount);
		uniformBufferCube().view = VulkanPlayground::CalcViewMatrix(scenePos, rotateCameraAmount + (MathsContants::pi<float> * 0.5f), 0);	// Sync scene with model
		uniformBufferCube.CopyToDevice(system);

		glm::vec3 viewPos = glm::rotateY(GetCameraPos().GetPosition(), rotateCameraAmount);
		mvpUBO().view = VulkanPlayground::CalcViewMatrix(viewPos, -rotateCameraAmount, 0);
	}

private:
	int showReflection = 1;	// Push constant variable
	std::vector<Model> models;
	Pipeline pipelineModel;
	Descriptor descriptorModel;
	Texture cubeTexture;

	Model cubeModel;	
	Pipeline pipelineCube;
	Descriptor descriptorCube;
	UBO<MVP> uniformBufferCube;
	Buffer vertexBufferCube;
	
	size_t curModel = 0;
	bool rotateCamera;
	float rotateCameraAmount;
	bool showModel, showSkybox;

};

DECLARE_APP(CubeMapCubeMapPlusModels)
