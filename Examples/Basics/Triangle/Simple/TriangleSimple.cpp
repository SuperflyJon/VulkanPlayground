#include <VulkanPlayground\Includes.h>

class TriangleSimpleApp : public VulkanApplication
{
	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		pipeline.LoadShader(system, "TriangleSimple");
		pipeline.SetupVertexDescription({ {1, Attribs::Type::Position, VK_FORMAT_R32G32_SFLOAT} });
		CreatePipeline(system, renderPass, pipeline, workingExtent, "TriangleSimple");

		system.CreateGpuBuffer<float>(system, vertexBuffer, { -.8f, 0.8f,  0.8f, 0.8f,  0.0f, -.8f }, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Triangle data");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer);
		vertexBuffer.Bind(commandBuffer);
		vkCmdDraw(commandBuffer, (uint32_t)vertexBuffer.GetBufferSize() / pipeline.GetStride(), 1, 0, 0);
	}

	Pipeline pipeline;
	Buffer vertexBuffer;
};

DECLARE_APP(TriangleSimple)
