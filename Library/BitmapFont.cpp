#include "stdafx.h"
#include "BitmapFont.h"
#include "Application.h"
#include "SwapChain.h"
#include "System.h"
#include <numeric>
#include "Fonts/BitmapFontInternal.h"
#include "Utils/StrUtil.h"
#include "../Library/VulkanPlayground/PixelData.h"
#include "Fonts/CharData.h"

uint32_t Vulkan2DFont::CurBufCharCount() const
{
	return (uint32_t)(fontBuffer.GetBufferSize() / sizeof(PosData));
}

PosData& Vulkan2DFont::GetNextPosData()
{
	if (pData - (PosData*)vertexMemory >= bufferSize)
		throw std::runtime_error("Buffer overrun accessing print data");

	return *pData++;
}

void Vulkan2DFont::RecordCharacter(char32_t character, float x, float y, const glm::vec3& colour, float scale)
{
	const CharData& charData = GetChar(character);
	PosData &data = GetNextPosData();
	data.pos.x = x;
	data.pos.y = y - (charData.Bitmap_top() * scale);
	data.size = glm::vec2(charData.Width(), charData.Rows()) * scale;
	data.textorg = glm::uvec2((short)charData.OffsetX(), (short)charData.OffsetY());
	data.textoff = glm::uvec2((short)charData.Width(), (short)charData.Rows());
	data.colour = colour;
	data.rgbColours = charData.Coloured() ? 1 : 0;
}

void Vulkan2DFont::SetupBackdrop(size_t x, size_t y, size_t backdropWidth, size_t backdropHeight, const glm::vec3& backgroundColour, VerticalAlignment verticalAlignment)
{	// Make a rougth background effect
	const CharData& charData = GetChar('.');
	PosData& data = GetNextPosData();
	data.pos = glm::vec2((float)x, (verticalAlignment == VerticalAlignment::Up) ? (float)y - backdropHeight : (float)y);
	data.size = glm::vec2(backdropWidth, backdropHeight);
	data.textorg = glm::uvec2((short)charData.OffsetX() + (charData.Width() + 1) / 2, (short)charData.OffsetY());
	data.textoff = glm::uvec2(1, 1);
	data.colour = backgroundColour;
	data.rgbColours = charData.Coloured() ? 1 : 0;
}

void Vulkan2DFont::SetupBuffers()
{
	if (bufferSize > 0 && (!fontBuffer.Created() || bufferSize != CurBufCharCount()))
	{
		if (fontBuffer.Created())
			system.DeviceWaitIdle();	// Might be in use in command buffer

		fontBuffer.Create(system, sizeof(PosData) * bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "vertexData");
		vertexMemory = fontBuffer.Map(system.GetDevice());
		pData = (PosData*)vertexMemory;

		if (bufferSize < CurBufCharCount())
		{	// Zero off any extra unused characters
			for (size_t extra = bufferSize; extra < CurBufCharCount(); extra++)
			{
				pData[extra].size = glm::vec2();
				bufferSize++;
			}
		}
	}
}

void Vulkan2DFont::Setup(VulkanApplication& theApp, VkExtent2D workingExtent, const RenderPass& renderPass)
{
	if (!IsValid())
	{
		PixelData pd(bitmapWidth, bitmapHeight, 1);
		pd.pixelData = bitmapData.get();
		texture.Create2dTexture(system, pd, anyColourGlyphs ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8_UNORM, faceName + " Font Texture");
		pd.pixelData = nullptr;	// Data managed by data member

		descriptor.AddUniformBuffer(system, 0, uboProj, "Projection", VK_SHADER_STAGE_GEOMETRY_BIT);
		descriptor.AddTexture(2, texture);
		theApp.CreateDescriptor(system, descriptor, "Font");

		std::vector<Attribs::Attrib> attribs{
			{0, Attribs::Type::Misc, VK_FORMAT_R32G32_SFLOAT},	//position
			{1, Attribs::Type::Misc, VK_FORMAT_R32G32_SFLOAT},	//size
			{2, Attribs::Type::Misc, VK_FORMAT_R32G32_UINT},	//textorg
			{3, Attribs::Type::Misc, VK_FORMAT_R32G32_UINT},	//textoff
			{4, Attribs::Type::Misc, VK_FORMAT_R32G32B32_SFLOAT},	//colour
			{5, Attribs::Type::Misc, VK_FORMAT_R32_SINT}	//coloured
		};
		pipeline.SetupVertexDescription(attribs);
		pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
		pipeline.LoadShaderWithGeomShader(system, "Font", "");
		pipeline.EnableBlending();
		pipeline.SetDepthTestEnabled(false);
		theApp.CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Font");

		uboProj().projection = glm::ortho(0.0f, (float)workingExtent.width, 0.0f, (float)workingExtent.height);
		uboProj().texWidth = bitmapWidth;
		uboProj().texHeight = bitmapHeight;
		uboProj.CopyToDevice(system);
	}
}

void Vulkan2DFont::Draw(VkCommandBuffer commandBuffer)
{
	if (bufferSize == 0)
		return;	// Nothing to draw in this font

	system.DebugInsertLabel(commandBuffer, faceName, { 0.0f, 0.0f, 1.0f });

	fontBuffer.Bind(commandBuffer);
	pipeline.Bind(commandBuffer, descriptor);
	vkCmdDraw(commandBuffer, bufferSize, 1, 0, 0);
}
