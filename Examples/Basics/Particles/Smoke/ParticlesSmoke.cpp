#include <VulkanPlayground\Includes.h>
#include "..\ParticleSystem.h"

class ParticlesSmokeApp : public VulkanApplication3D
{
	#include "..\..\models\quad.vertices"

public:
	ParticlesSmokeApp()
	{
		TidyObjectOnExit(particles);
	}

	void ResetScene() override
	{
		testBigParticles = false;
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

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 3, viewport, "size");
		descriptor.AddTexture(system, 1, smoke, VulkanPlayground::GetModelFile("Basics", "particle_smoke.ktx"));
		CreateDescriptor(system, descriptor, "Draw");

		pipeline.AddPushConstant(system, sizeof(int), VK_SHADER_STAGE_FRAGMENT_BIT);
		pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
		pipeline.SetDepthWriteEnabled(false);
		pipeline.EnableBlending(VK_BLEND_FACTOR_DST_ALPHA, VK_BLEND_FACTOR_ONE);

		pipeline.LoadShaderDiffNames(system, "Particles", "Smoke");

		std::vector<Attribs::Attrib> attribs{
			{0, Attribs::Type::Position, VK_FORMAT_R32G32B32_SFLOAT},
			{1, Attribs::Type::Misc, VK_FORMAT_R32_SFLOAT},	// Alpha
			{2, Attribs::Type::Misc, VK_FORMAT_R32_SFLOAT},	// Size
			{3, Attribs::Type::Misc, VK_FORMAT_R32_SFLOAT},	// Rotation
			{4, Attribs::Type::Misc, VK_FORMAT_R32_SINT}	// Type
		};
		pipeline.SetupVertexDescription(attribs);
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Main");

		constexpr auto PARTICLE_COUNT = 700;
		particles.Prepare(system, testBigParticles ? 3 : PARTICLE_COUNT, testBigParticles ? 2 : 0);

		viewport().X = (float)windowWidth;
		viewport.CopyToDevice(system);

		testDescriptor.AddUniformBuffer(system, 0, testUBO, "MVP");
		testDescriptor.AddTexture(system, 1, texture, VulkanPlayground::GetModelFile("Basics", "metalplate01_rgba.ktx"));
		CreateDescriptor(system, testDescriptor, "Drawing");

		testPipeline.LoadShader(system, "TestTexture");
		testPipeline.SetupVertexDescription(Attribs::PosTex);
		CreatePipeline(system, renderPass, testPipeline, testDescriptor, workingExtent, "Scene");

		system.CreateGpuBuffer(system, vertexBuffer, quadVerticesPT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Vertices");
	}

	void UpdateScene(VulkanSystem& system, float frameTime) override
	{
		if (!pause)
			particles.Update(frameTime);

		testUBO() = mvpUBO();
		testUBO().model = glm::scale(testUBO().model, glm::vec3(45.0f));
		testUBO.CopyToDevice(system);

		mvpUBO().model = glm::translate(mvpUBO().model, glm::vec3(0, 15, 0));
	}

	void ProcessKeyPresses(const EventData& eventData) override
	{
		if (eventData.KeyPressed('T'))
		{
			testBigParticles = !testBigParticles;
			RecreateObjects();
		}
		if (eventData.KeyPressed(' '))
			pause = !pause;
	}

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		testPipeline.Bind(commandBuffer, testDescriptor);
		vertexBuffer.Bind(commandBuffer);
		vkCmdDraw(commandBuffer, Attribs::NumVertices(quadVerticesPT, Attribs::PosTex), 1, 0, 0);

		int draw = SMOKE_PARTICLE;
		pipeline.PushConstant(commandBuffer, &draw);
		pipeline.Bind(commandBuffer, descriptor);
		particles.Draw(commandBuffer);
	}

private:
	Descriptor descriptor;
	Pipeline pipeline;
	Texture smoke;
	Particles particles;
	struct Viewport
	{
		float X;
	};
	UBO<Viewport> viewport;

	UBO<MVP> testUBO;
	Pipeline testPipeline;
	Descriptor testDescriptor;
	Texture texture;
	Buffer vertexBuffer;
	bool pause, testBigParticles = false;
};

DECLARE_APP(ParticlesSmoke)
