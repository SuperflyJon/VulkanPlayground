#include <VulkanPlayground\Includes.h>

class TextureLoDApp : public VulkanApplication3DLight
{
	#include "..\..\models\quad.vertices"

	void ResetScene() override
	{
		CalcPositionMatrixMoveBack(1);
		SetupLighting({ -24, -1, 7 }, 0.5f, 0.5f, 0.75f, 32.0f);	// Set lighting to shine on the model
		loDBias = 0.0f;
		pipeline.Recreate();
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 2, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);
		descriptor.AddTexture(system, 1, texture, VulkanPlayground::GetModelFile("Basics", "metalplate01_rgba.ktx"));
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.LoadShaderDiffNames(system, "Texture", "LoD");
		pipeline.SetupVertexDescription(Attribs::PosNormTex);
		pipeline.AddPushConstant(system, sizeof(loDBias), VK_SHADER_STAGE_FRAGMENT_BIT);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");

		system.CreateGpuBuffer(system, vertexBuffer, quadVertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Vertices");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.PushConstant(commandBuffer, &loDBias);
		pipeline.Bind(commandBuffer, descriptor);
		vertexBuffer.Bind(commandBuffer);
		vkCmdDraw(commandBuffer, Attribs::NumVertices(quadVertices, Attribs::PosNormTex), 1, 0, 0);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			if (!eventData.ShiftPressed())
			{
				if (loDBias < (float)texture.GetMipLevels())
					loDBias += 1.0f;
			}
			else
			{
				if (loDBias > 0)
					loDBias -= 1.0f;
			}
			
			pipeline.Recreate();	// Recreate pipeline (to use new bias value)
		}
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Texture texture;
	Buffer vertexBuffer;
	float loDBias;
};

DECLARE_APP(TextureLoD)
