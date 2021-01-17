#include <VulkanPlayground\Includes.h>

class CubeMapCubeMapApp : public VulkanApplication3D
{
	void ResetScene() override
	{
		// Turn image upside down as samplerCube expects that
		mvpUBO().model = glm::rotate(mvpUBO().model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "cube.obj"), Attribs::Pos);

		cubeTexture.LoadCubeMap(system, VulkanPlayground::GetModelFile("Basics", "cubemap_yokohama_bc3.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddTexture(1, cubeTexture);
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetWindingOrder(VK_FRONT_FACE_CLOCKWISE);	// Drawing on inside of cube
		pipeline.SetDepthTestOp(VK_COMPARE_OP_LESS_OR_EQUAL);	// Maximum depth values for skybox so have to set test to <= 1 others it won't show up correctly
		pipeline.SetupVertexDescription(Attribs::Pos);
		pipeline.LoadShader(system, "CubeMap");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "CubeMap");

	}
	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);
	}

private:
	Model model;
	Pipeline pipeline;
	Descriptor descriptor;
	Texture cubeTexture;
};

DECLARE_APP(CubeMapCubeMap)
