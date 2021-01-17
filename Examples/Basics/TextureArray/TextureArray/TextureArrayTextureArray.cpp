#include <VulkanPlayground\Includes.h>

class TextureArrayTextureArrayApp : public VulkanApplication3D
{
	static constexpr auto GRID_SIZE = 5;

	struct UBO_mt
	{
		glm::mat4 model;
		int textureNumber;
	};

	void ResetScene() override
	{
		CalcPositionMatrixMoveBack(10.0f, model.GetModelSize());
		worldPos.Reset({ GRID_SIZE, GRID_SIZE, GRID_SIZE }, 0, 0);	// Centre cubes
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "cube.obj"), Attribs::PosTex);

		Texture::LoadTextureArrayIntoTextures(system, textures, VulkanPlayground::GetModelFile("Basics", "texturearray_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddDynamicUniformBuffer(system, 1, dynamicUniformBuffer, GRID_SIZE * GRID_SIZE * GRID_SIZE, "Cubes");
		descriptor.AddTextureSampler(2);
		descriptor.AddTextureArray(3, textures.data(), (uint32_t)textures.size());
		CreateDescriptor(system, descriptor, "Drawing");

		Shader &shader = pipeline.LoadShader(system, "TextureArray");
		shader.SetSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, textures.size());
		pipeline.SetupVertexDescription(Attribs::PosTex);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");

		uint32_t index = 0;
		for (uint32_t x = 0; x < GRID_SIZE; x++)
		{
			for (uint32_t y = 0; y < GRID_SIZE; y++)
			{
				for (uint32_t z = 0; z < GRID_SIZE; z++)
				{
					dynamicUniformBuffer(index).model = glm::translate(glm::mat4(), glm::vec3(x * 2.5, y * 2.5, z * 2.5));
					dynamicUniformBuffer(index).textureNumber = index % textures.size();
					index++;
				}
			}
		}
		dynamicUniformBuffer.CopyToDevice(system);
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		for (uint32_t buff = 0; buff < GRID_SIZE * GRID_SIZE * GRID_SIZE; buff++)
		{
			pipeline.Bind(commandBuffer, descriptor.GetDescriptorSet(), buff * dynamicUniformBuffer.GetBlockSize());
			model.Draw(commandBuffer, (buff == 0));
		}
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Model model;
	std::vector<Texture> textures;
	DynamicUBO<UBO_mt> dynamicUniformBuffer;
};

DECLARE_APP(TextureArrayTextureArray)
