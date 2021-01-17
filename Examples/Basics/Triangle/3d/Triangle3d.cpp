#include <VulkanPlayground\Includes.h>

class Triangle3dApp : public VulkanApplication3D
{
	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.LoadShader(system, "Triangle3d");
		pipeline.SetupVertexDescription({ {1, Attribs::Type::Position, VK_FORMAT_R32G32B32_SFLOAT}, {2, Attribs::Type::Colour, VK_FORMAT_R32G32B32_SFLOAT} });
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Triangle3d");

		system.CreateGpuBuffer<float>(system, vertexBuffer, {
			-.8f, 0.8f, .0f,  1.0f, 0.0f, 0.0f,
			0.0f, -.8f, .0f,  0.0f, 0.0f, 1.0f,
			0.8f, 0.8f, .0f,  0.0f, 1.0f, 0.0f,
			}, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Triangle data");
	}

	void ResetScene() override
	{
		// Move camera to point at triangle
		GetCameraPos().GetPosition().x = -2.5;
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		vertexBuffer.Bind(commandBuffer);
		vkCmdDraw(commandBuffer, (uint32_t)vertexBuffer.GetBufferSize() / pipeline.GetStride(), 1, 0, 0);
	}

	Pipeline pipeline;
	Descriptor descriptor;
	Buffer vertexBuffer;
};

DECLARE_APP(Triangle3d)
