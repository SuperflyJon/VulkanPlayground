#include <VulkanPlayground\Includes.h>
#include <VulkanPlayground\PixelData.h>

extern void GenerateNoise(bool fractal, float noiseScale, PixelData* pd);

class Textures3dNoise3dApp : public VulkanApplication3D
{
#include "..\..\models\quad.vertices"
	const int numImages = 128;

	struct UBO_d
	{
		float depth;
	};

	void ResetScene()
	{
		pos = 0;
		pause = false;
		superSlow = false;
	}

	void MakeSomeNoise()
	{
		if (noiseData == nullptr)
		{
			float noiseScale;
			if (useFractalNoise)
				noiseScale = static_cast<float>(rand() % 10) + 4.0f;
			else
				noiseScale = 20.0f;
			noiseData = new PixelData(numImages, numImages, numImages);
			GenerateNoise(useFractalNoise, noiseScale, noiseData);
		}
		else
			noiseData->mipLevels = numImages;	// Reset field as gets set to 1 when uploaded as a single 3d image set
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		MakeSomeNoise();
		texture.SetAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		texture.Create3dTexture(system, *noiseData, VK_FORMAT_R8_UNORM, "noise");

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 1, uniformBuffer, "depth");
		descriptor.AddTexture(2, texture);
		CreateDescriptor(system, descriptor, "Drawing");

		pipeline.SetupVertexDescription(Attribs::PosNormTex);
		pipeline.LoadShader(system, "Noise3d");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Scene");

		system.CreateGpuBuffer(system, vertexBuffer, quadVertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Buff");
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.GetBuffer(), VulkanPlayground::zeroOffset);
		pipeline.Bind(commandBuffer, descriptor);
		vkCmdDraw(commandBuffer, Attribs::NumVertices(quadVertices, Attribs::PosNormTex), 1, 0, 0);
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed(' '))
			pause = !pause;

		if (eventData.KeyPressed('T'))
			superSlow = !superSlow;

		if (eventData.KeyPressed('M'))
		{
			useFractalNoise = !useFractalNoise;
			delete noiseData;
			noiseData = nullptr;
			RecreateObjects();
		}
	}

	void UpdateScene(VulkanSystem& system, float frameTime) override
	{
		if (!pause)
		{
			pos += frameTime / (superSlow ? 200 : 10);
			if (pos > 1)
				pos = 0;
		}
		uniformBuffer().depth = pos;
		uniformBuffer.CopyToDevice(system);
	}

private:
	Pipeline pipeline;
	Descriptor descriptor;
	Buffer vertexBuffer;
	Texture texture;
	UBO<UBO_d> uniformBuffer;

	float pos;
	bool pause;
	bool superSlow;
	bool useFractalNoise = false;
	PixelData* noiseData = nullptr;
};

DECLARE_APP(Textures3dNoise3d)
