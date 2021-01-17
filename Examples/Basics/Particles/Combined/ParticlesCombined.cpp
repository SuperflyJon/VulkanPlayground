#include <VulkanPlayground\Includes.h>
#include <random>
#include "..\ParticleSystem.h"

class ParticlesCombinedApp : public VulkanApplication3DLight
{
	struct VertInfo
	{
		glm::vec3 lightPos;
		float pad1;
		glm::vec3 viewPos;
	};
	
public:
	ParticlesCombinedApp()
	{
		TidyObjectOnExit(particles);
	}

	void ResetScene() override
	{
		CalcPositionMatrix({ 0.0f, 0.0f, 0.0f }, { -70.0f, -45.0f, 0 }, 0, 30);
		SetupLighting({ 0.0f, -35.0f, 0.0f }, 0.0f, 0.5f, 0.1f, 32.0f, model.GetModelSize());
	}

	glm::vec4 GetClearColour() const override
	{
		return { 0, 0, 0, 1 };
	}

	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override
	{
		model.CorrectDodgyModelOnLoad();
		model.LoadToGpu(system, VulkanPlayground::GetModelFile("Basics", "fireplace.obj"), Attribs::PosNormTexTanBitan);

		descriptor.AddUniformBuffer(system, 0, mvpUBO, "MVP");
		descriptor.AddUniformBuffer(system, 1, vertInfo, "vertInfo");
		
		descriptor.AddTexture(system, 3, texture, VulkanPlayground::GetModelFile("Basics", "fireplace_colormap_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		descriptor.AddTexture(system, 4, normalMap, VulkanPlayground::GetModelFile("Basics", "fireplace_normalmap_bc3_unorm.ktx"), VK_FORMAT_BC3_UNORM_BLOCK);
		descriptor.AddUniformBuffer(system, 2, lightUBO, "Lighting", VK_SHADER_STAGE_FRAGMENT_BIT);
		CreateDescriptor(system, descriptor, "Draw");

		pipeline.SetupVertexDescription(Attribs::PosNormTexTanBitan);
		pipeline.LoadShader(system, "NormalMap");
		CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Main");

		smoke.SetAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);	// Avoid edges bleeding over
		flame.SetAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

		fireDescriptor.AddUniformBuffer(system, 0, fireUniformBuffer, "Fire");
		fireDescriptor.AddUniformBuffer(system, 3, viewport, "size");
		fireDescriptor.AddTexture(system, 1, flame, VulkanPlayground::GetModelFile("Basics", "particle_flame.ktx"));
		fireDescriptor.AddTexture(system, 2, smoke, VulkanPlayground::GetModelFile("Basics", "particle_smoke.ktx"));
		CreateDescriptor(system, fireDescriptor, "Fire");

		std::vector<Attribs::Attrib> fireAttribs{
			{0, Attribs::Type::Position, VK_FORMAT_R32G32B32_SFLOAT},
			{1, Attribs::Type::Misc, VK_FORMAT_R32_SFLOAT},	// Alpha
			{2, Attribs::Type::Misc, VK_FORMAT_R32_SFLOAT},	// Size
			{3, Attribs::Type::Misc, VK_FORMAT_R32_SFLOAT},	// Rotation
			{4, Attribs::Type::Misc, VK_FORMAT_R32_SINT}	// Type
		};
		fireGraphicsPipeline.SetupVertexDescription(fireAttribs);
		fireGraphicsPipeline.AddPushConstant(system, sizeof(int), VK_SHADER_STAGE_FRAGMENT_BIT);
		fireGraphicsPipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
		fireGraphicsPipeline.SetDepthWriteEnabled(false);
		fireGraphicsPipeline.EnableBlending(VK_BLEND_FACTOR_DST_ALPHA, VK_BLEND_FACTOR_ONE);
		fireGraphicsPipeline.LoadShaderDiffNames(system, "Particles", "Fire");
		CreatePipeline(system, renderPass, fireGraphicsPipeline, fireDescriptor, workingExtent, "Fire");

constexpr auto PARTICLE_COUNT = 500;
		particles.Prepare(system, PARTICLE_COUNT);

		viewport().X = (float)windowWidth;
		viewport.CopyToDevice(system);
	};

	void DrawScene(VkCommandBuffer commandBuffer) override
	{
		pipeline.Bind(commandBuffer, descriptor);
		model.Draw(commandBuffer);

		// Draw fire in 2 passes to get the blending okay
		fireGraphicsPipeline.Bind(commandBuffer, fireDescriptor.GetDescriptorSet());
		int draw = FLAME_PARTICLE;
		fireGraphicsPipeline.PushConstant(commandBuffer, &draw);
		particles.Draw(commandBuffer);
		draw = SMOKE_PARTICLE;
		fireGraphicsPipeline.PushConstant(commandBuffer, &draw);
		particles.Draw(commandBuffer);
	}

	void UpdateScene(VulkanSystem& system, float frameTime) override
	{		
		fireUniformBuffer() = mvpUBO();
		fireUniformBuffer().model = glm::translate(fireUniformBuffer().model, glm::vec3(-1.4f, 5.0f, 0.4f));	// Center fire on logs
		fireUniformBuffer.CopyToDevice(system);

		mvpUBO().model = glm::scale(mvpUBO().model, glm::vec3(10.0f));

		particles.Update(frameTime);

		vertInfo().lightPos = lightUBO().lightPos;
		vertInfo().lightPos.x += jitterX.Vary(frameTime) * 25.0f;
		vertInfo().lightPos.z += jitterZ.Vary(frameTime) * 25.0f;
		vertInfo().viewPos = lightUBO().viewPos;
		vertInfo.CopyToDevice(system);
	}

private:
	Descriptor descriptor;
	Pipeline pipeline;
	Model model;
	Texture texture, normalMap;
	UBO<VertInfo> vertInfo;

	UBO<MVP> fireUniformBuffer;
	Texture flame, smoke;
	Particles particles;
	Pipeline fireGraphicsPipeline;
	Descriptor fireDescriptor;
	struct Viewport
	{
		float X;
	};
	UBO<Viewport> viewport;
	Jitter jitterX, jitterZ;
};

DECLARE_APP(ParticlesCombined)
