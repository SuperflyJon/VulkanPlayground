#include <VulkanPlayground\Includes.h>

class TriangleLightApp : public VulkanApplication3DLight
{
	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 1, lightUBO, "Light Info", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetCullMode(VK_CULL_MODE_NONE);	// Show both sides of triangle
		pipeline.LoadShader(system, "TriangleLight");
		pipeline.SetupVertexDescription(Attribs::PosNormCol);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Triangle3d");

		system.CreateGpuBuffer<float>(system, vertexBuffer, {
			-.8f, 0.8f, .0f,  .0f, .0f, -1.0f,  0.0f, 0.0f, 1.0f,
			0.0f, -.8f, .0f,  .0f, .0f, -1.0f,  0.0f, 0.0f, 1.0f,
			0.8f, 0.8f, .0f,  .0f, .0f, -1.0f,  0.0f, 0.0f, 1.0f,
			}, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Triangle data");
	}

	void ResetScene() override
	{
		CalcPositionMatrixMoveBack(2.5f);
		SetupLighting({ -22.0, 5.0, 5.0 }, 0.3f, 0.5f, 0.3f, 70.0f);
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

DECLARE_APP(TriangleLight)
