#include <VulkanPlayground\Includes.h>
#include "..\ParticleSystem.h"

class ParticlesFireApp : public VulkanApplication3D
{
public:
	ParticlesFireApp()
	{
		TidyObjectOnExit(particles);
	}

	void ResetScene() override
	{
		pause = false;
		CalcPositionMatrix({ 0.0f, 0.0f, 0.0f }, { -50.0f, 0.0f, 0 }, 0, 0);
	}

	glm::vec4 GetClearColour() const override
	{
		return { clearColour.r, clearColour.g, clearColour.b, 1 };
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		smoke.SetAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);	// Avoid edges bleeding over
		flame.SetAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 3, viewport, "size");
		
		descriptor.AddTexture(system, 1, flame, VulkanPlayground::GetModelFile("Basics", "particle_flame.ktx"));
		descriptor.AddTexture(system, 2, smoke, VulkanPlayground::GetModelFile("Basics", "particle_smoke.ktx"));
		CreateDescriptor(system, descriptor, "Draw");

		std::vector<Attribs::Attrib> fireAttribs{
			{0, Attribs::Type::Position, VK_FORMAT_R32G32B32_SFLOAT},
			{1, Attribs::Type::Misc, VK_FORMAT_R32_SFLOAT},	// Alpha
			{2, Attribs::Type::Misc, VK_FORMAT_R32_SFLOAT},	// Size
			{3, Attribs::Type::Misc, VK_FORMAT_R32_SFLOAT},	// Rotation
			{4, Attribs::Type::Misc, VK_FORMAT_R32_SINT}	// Type
		};
		pipeline.SetupVertexDescription(fireAttribs);
		pipeline.AddPushConstant(system, sizeof(int), VK_SHADER_STAGE_FRAGMENT_BIT);
		pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
		pipeline.SetDepthWriteEnabled(false);
		pipeline.EnableBlending(VK_BLEND_FACTOR_DST_ALPHA, VK_BLEND_FACTOR_ONE);
		pipeline.LoadShaderDiffNames(system, "Particles", "Fire");
		pipeline.AddPushConstant(system, sizeof(int), VK_SHADER_STAGE_FRAGMENT_BIT);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Main");

constexpr auto PARTICLE_COUNT = 500;
		particles.Prepare(system, PARTICLE_COUNT);

		viewport().X = (float)windowWidth;
		viewport.CopyToDevice(system);
	}

	void UpdateScene(VulkanSystem& /*system*/, float frameTime) override
	{
		if (!pause)
			particles.Update(frameTime);

		mvpUBO().model = glm::translate(mvpUBO().model, glm::vec3(0, 20, 0));
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed(' '))
			pause = !pause;
		if (eventData.KeyPressed('5'))
		{
			showSmoke = !showSmoke;
			RedrawScene();
		}
		if (eventData.KeyPressed('6'))
		{
			showFlame = !showFlame;
			RedrawScene();
		}
	}
	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		int draw;
		if (showFlame)
		{
			draw = FLAME_PARTICLE;
			pipeline.PushConstant(commandBuffer, &draw);
			particles.Draw(commandBuffer);
		}
		if (showSmoke)
		{
			draw = SMOKE_PARTICLE;
			pipeline.PushConstant(commandBuffer, &draw);
			particles.Draw(commandBuffer);
		}
	}

private:
	Descriptor descriptor;
	Pipeline pipeline;
	Texture flame, smoke;
	Particles particles;

	struct Viewport
	{
		float X;
	};
	UBO<Viewport> viewport;
	bool pause, showFlame = true, showSmoke = true;
};

DECLARE_APP(ParticlesFire)
