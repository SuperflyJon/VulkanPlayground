#include <VulkanPlayground\Includes.h>
#include <glm\gtx\rotate_vector.hpp>

class Textures3dMoreQuads3dApp : public VulkanApplication3D
{
	#include "..\..\models\quad.vertices"

	static constexpr auto NUM_QUADS = 21;

	struct UBO_m
	{
		glm::mat4 model[NUM_QUADS];
	};

	void ResetScene() override
	{
		CalcPositionMatrixMoveBack(2.0f + numQuads * 1.8f);
		cameraPos.SetYaw(-33);
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		texture.SetAddressMode(repeatAddressMode ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		texture.Load3dTexture(system, VulkanPlayground::GetModelFile("Basics", "texturearray_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		system.CreateGpuBuffer(system, vertexBuffer, quadVertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Vertices");

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 1, objects, "Objects");
		descriptor.AddTexture(2, texture);
		CreateDescriptor(system, descriptor, "Drawing");

		Shader &shader = pipeline.LoadShader(system, "MoreQuads3d");
		shader.SetSpecializationConstant(VK_SHADER_STAGE_VERTEX_BIT, 1, (numQuads + 1) * texture.GetNumLayers());
		pipeline.SetupVertexDescription(Attribs::PosNormTex);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");

		for (uint32_t index = 0; index < (numQuads + 1) * texture.GetNumLayers(); index++)
		{
			objects().model[index] = glm::translate(glm::mat4(), glm::vec3(index * 0.5, 0, 0));
			objects().model[index] = glm::rotate(objects().model[index], glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));	// Turn arond so flat
		}
		objects.CopyToDevice(system);
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.GetBuffer(), VulkanPlayground::zeroOffset);
		pipeline.Bind(commandBuffer, descriptor);
		vkCmdDraw(commandBuffer, Attribs::NumVertices(quadVertices, Attribs::PosNormTex), (numQuads + 1) * texture.GetNumLayers(), 0, 0);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			numQuads = (numQuads + 1) % 3;
			ResetScene();
			RecreateObjects();
		}
		if (eventData.KeyPressed('M'))
		{
			repeatAddressMode = !repeatAddressMode;
			RecreateObjects();
		}
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Buffer vertexBuffer;
	Texture texture;
	UBO<UBO_m> objects;

	int numQuads = 1;
	bool repeatAddressMode = false;
};

DECLARE_APP(Textures3dMoreQuads3d)
