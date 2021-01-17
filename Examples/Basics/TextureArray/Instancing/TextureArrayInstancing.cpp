#include <VulkanPlayground\Includes.h>

class TextureArrayInstancingApp : public VulkanApplication3D
{
	static constexpr auto GRID_SIZE = 5;

	struct UBO_m
	{
		glm::mat4 model[GRID_SIZE * GRID_SIZE * GRID_SIZE];
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
		descriptor.AddUniformBuffer(system, 1, objects, "Objects");
		descriptor.AddTextureSampler(2);
		descriptor.AddTextureArray(3, textures.data(), (uint32_t)textures.size());
		CreateDescriptor(system, descriptor, "Drawing");

		Shader &shader = pipeline.LoadShaderDiffNames(system, "Instancing", "TextureArray");
		shader.SetSpecializationConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, textures.size());
		shader.SetSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, textures.size());
		shader.SetSpecializationConstant(VK_SHADER_STAGE_VERTEX_BIT, 1, GRID_SIZE * GRID_SIZE * GRID_SIZE);
		pipeline.SetupVertexDescription(Attribs::PosTex);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");

		uint32_t index = 0;
		for (uint32_t x = 0; x < GRID_SIZE; x++)
		{
			for (uint32_t y = 0; y < GRID_SIZE; y++)
			{
				for (uint32_t z = 0; z < GRID_SIZE; z++)
				{
					objects().model[index] = glm::translate(glm::mat4(), glm::vec3(x * 2.5, y * 2.5, z * 2.5));
					index++;
				}
			}
		}
		objects.CopyToDevice(system);
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer, true, GRID_SIZE * GRID_SIZE * GRID_SIZE);
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Model model;
	std::vector<Texture> textures;
	UBO<UBO_m> objects;
};

DECLARE_APP(TextureArrayInstancing)
