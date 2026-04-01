#pragma once

#include "Descriptor.h"
#include "Pipeline.h"
#include "Image.h"

#include "Fonts/FontData.h"

class TypeSetter;
class CharData;
class Heights;
struct PrintInfo;

struct PosData
{
	glm::vec2 pos, size;
	glm::uvec2 textorg, textoff;
	glm::vec3 colour;
	int rgbColours;
};

class Vulkan2DFont : public FontData, public ITidy
{
public:
	Vulkan2DFont(VulkanSystem& system, const std::string& faceName = "Arial", int fontSize = 24) : FontData(faceName, fontSize), system(system)
	{
	}

	void SetupBuffers() override;
	void RecordCharacter(char32_t character, float x, float y, const glm::vec3& colour, float scale) override;
	void SetupBackdrop(size_t x, size_t y, size_t backdropWidth, size_t backdropHeight, const glm::vec3& backgroundColour, VerticalAlignment verticalAlignment) override;
	uint32_t CurBufCharCount() const;

	virtual void Reset()
	{
		FontData::Reset();

		// Reset data pointer
		pData = (PosData*)vertexMemory;
	}

	void Setup(VulkanApplication& theApp, VkExtent2D workingExtent, const RenderPass& renderPass);
	void Draw(VkCommandBuffer commandBuffer);

	bool IsValid() const override { return pipeline.Created(); }

	void Tidy(VulkanSystem& _system) override
	{
		fontBuffer.DestroyBuffer(_system);
	}

private:
	PosData& GetNextPosData();

	VulkanSystem& system;

	Descriptor descriptor;
	Pipeline pipeline;
	Texture texture;

	struct UboProj
	{
		glm::mat4 projection;
		int texWidth, texHeight;
	};
	UBO<UboProj> uboProj;

	Buffer fontBuffer;
	void* vertexMemory = nullptr;
	PosData* pData = nullptr;
};
