#include <VulkanPlayground\Includes.h>

class CubeMapBoxedApp : public VulkanApplication3D
{
	static constexpr auto TEXTURE_ARRAY_SIZE = 6;
	#include "..\..\models\quad.vertices"

	struct UBO_mt
	{
		glm::mat4 model;
		int textureNumber;
	};

	void ResetScene() override
	{
		cameraPos.Reset(glm::vec3(), 180, 0);	// Turn around to match "real" cubemap
		showBoxed = true;
		RecreateObjects();
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		Texture::LoadCubeIntoTextures(system, cubeTextures, VulkanPlayground::GetModelFile("Basics", "cubemap_yokohama_bc3.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddDynamicUniformBuffer(system, 1, dynamicUniformBuffer, TEXTURE_ARRAY_SIZE, "BoxDyn");
		descriptor.AddTextureSampler(2);
		descriptor.AddTextureArray(3, cubeTextures, TEXTURE_ARRAY_SIZE);
		CreateDescriptor(system, descriptor, "Drawing");

		Shader &shader = pipeline.LoadShader(system, "TextureArray");
		shader.SetSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, TEXTURE_ARRAY_SIZE);
		pipeline.SetupVertexDescription(Attribs::PosTex);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");
		
		system.CreateGpuBuffer(system, vertexBuffer, quadVerticesPT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Buf");

		// Reposition faces so looks like a boxed (or unboxed) cube
		std::vector<glm::vec3> rots;
		if (showBoxed)
			rots = { {0, 1, 0}, { 0, -1, 0 }, { 1, 0, 2 }, { -1, 0, 2 }, { 0, 2, 0 }, { 0, 0, 0 } };
		else
			rots = { {-1, 0, 1}, {-1, 0, -1}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 2}, {-1, 0, 0} };

		std::vector<int> xPos{ 1,-1, -2, 0, 0, 0 };
		std::vector<int> yPos{ 0, 0, 0, 0,-1, 1 };
	
		for (uint32_t index = 0; index < TEXTURE_ARRAY_SIZE; index++)
		{
			dynamicUniformBuffer(index).model = glm::mat4();
			if (!showBoxed)
				dynamicUniformBuffer(index).model = glm::translate(dynamicUniformBuffer(index).model, glm::vec3((float)xPos[index], 0, yPos[index]));
			dynamicUniformBuffer(index).model = glm::rotate(dynamicUniformBuffer(index).model, glm::radians(90 * rots[index].x), glm::vec3(1.0f, 0.0f, 0.0f));
			dynamicUniformBuffer(index).model = glm::rotate(dynamicUniformBuffer(index).model, glm::radians(90 * rots[index].y), glm::vec3(0.0f, 1.0f, 0.0f));
			dynamicUniformBuffer(index).model = glm::rotate(dynamicUniformBuffer(index).model, glm::radians(90 * rots[index].z), glm::vec3(0.0f, 0.0f, 1.0f));

			dynamicUniformBuffer(index).textureNumber = index;
		}

		dynamicUniformBuffer.CopyToDevice(system);
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.GetBuffer(), VulkanPlayground::zeroOffset);

		for (uint32_t buff = 0; buff < TEXTURE_ARRAY_SIZE; buff++)
		{
			pipeline.Bind(commandBuffer, descriptor.GetDescriptorSet(), buff * dynamicUniformBuffer.GetBlockSize());
			vkCmdDraw(commandBuffer, Attribs::NumVertices(quadVerticesPT, Attribs::PosTex), 1, 0, 0);
		}
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			showBoxed = !showBoxed;
			RecreateObjects();
		}
	}

private:
	bool showBoxed;
	Pipeline pipeline;
	Descriptor descriptor;
	Buffer vertexBuffer;
	Texture cubeTextures[TEXTURE_ARRAY_SIZE];
	DynamicUBO<UBO_mt> dynamicUniformBuffer;
};

DECLARE_APP(CubeMapBoxed)
