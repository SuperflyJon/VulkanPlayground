#include <VulkanPlayground\Includes.h>

class CubeMapTextureArrayApp : public VulkanApplication3D
{
	static constexpr auto TEXTURE_ARRAY_SIZE = 8;
	static constexpr auto GRID_SIZE = 5;

	struct UBO_mt
	{
		glm::mat4 model;
		int textureNumber;
	};

	void ResetScene() override
	{
		CalcPositionMatrixMoveBack(8.0f, model.GetModelSize());
		worldPos.Reset({ 5, 5, 0 }, 0, 0);	// Centre cubes
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "cube.obj"), Attribs::PosTex);

		for (uint32_t i = 1; i <= TEXTURE_ARRAY_SIZE; ++i)
		{
			std::stringstream ss;
			ss << "pic" << i << ".png";
			textures[i - 1].Load(system, VulkanPlayground::GetModelFile("Basics", ss.str()), VK_FORMAT_R8G8B8A8_UNORM, (i == 1) /* Create sampler in first image only */);
		}

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddDynamicUniformBuffer(system, 1, dynamicUniformBuffer, GRID_SIZE * GRID_SIZE, "Cubes");
		descriptor.AddTextureSampler(2);
		descriptor.AddTextureArray(3, textures, TEXTURE_ARRAY_SIZE);
		CreateDescriptor(system, descriptor, "Drawing");

		Shader &shader = pipeline.LoadShader(system, "TextureArray");
		shader.SetSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, TEXTURE_ARRAY_SIZE);
		pipeline.SetupVertexDescription(Attribs::PosTex);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");

		uint32_t index = 0;
		for (uint32_t x = 0; x < GRID_SIZE; x++)
		{
			for (uint32_t y = 0; y < GRID_SIZE; y++)
			{
				dynamicUniformBuffer(index).model = glm::translate(glm::mat4(), glm::vec3(x * 2.5, y * 2.5, 0));
				dynamicUniformBuffer(index).textureNumber = index % TEXTURE_ARRAY_SIZE;
				index++;
			}
		}
		dynamicUniformBuffer.CopyToDevice(system);
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		for (uint32_t buff = 0; buff < GRID_SIZE * GRID_SIZE; buff++)
		{
			pipeline.Bind(commandBuffer, descriptor.GetDescriptorSet(), buff * dynamicUniformBuffer.GetBlockSize());
			model.Draw(commandBuffer, (buff == 0));
		}
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Model model;
	Texture textures[TEXTURE_ARRAY_SIZE];
	DynamicUBO<UBO_mt> dynamicUniformBuffer;
};

DECLARE_APP(CubeMapTextureArray)
