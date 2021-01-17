#include <VulkanPlayground\Includes.h>

class CubeMapUnboxedApp : public VulkanApplication3D
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
		CalcPositionMatrixMoveBack(5);
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

		// Reposition faces so looks like un-boxed cube (NB. Vulkan expects them in this order: front, back, up, down, right, left)
		std::vector<int> xPos{ 1, 3, 2, 2, 2, 0 };
		std::vector<int> yPos{ 1, 1, 0, 2, 1, 1 };
		for (uint32_t index = 0; index < TEXTURE_ARRAY_SIZE; index++)
		{
			dynamicUniformBuffer(index).model = glm::translate(glm::mat4(), glm::vec3((float)xPos[index] - 1.5, yPos[index] - 1, 0));
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

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Buffer vertexBuffer;
	Texture cubeTextures[TEXTURE_ARRAY_SIZE];
	DynamicUBO<UBO_mt> dynamicUniformBuffer;
};

DECLARE_APP(CubeMapUnboxed)
