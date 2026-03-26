#pragma once

#include "Descriptor.h"
#include "Pipeline.h"
#include "PixelData.h"
#include "Image.h"

class Packer;
class TypeSetter;
class CharData;
class Heights;
struct PrintInfo;

enum class HorizontalAlignment { Left, Right };
enum class VerticalAlignment { Up, Down };

struct PosData
{
	glm::vec2 pos, size;
	glm::uvec2 textorg, textoff;
	glm::vec3 colour;
	int coloured{ -1 };
};

class FreeTypeFont;

class FontData : public ITidy
{
public:
	FontData(const std::string& _faceName, int _fontSize);
	~FontData();

	const CharData& GetChar(char32_t character) const;
	int GetKerning(char32_t first, char32_t second) const;
	void SetupKerning(FreeTypeFont& ft_font, const std::u32string& text);

	virtual bool IsValid() const = 0;
	virtual void Reset()
	{
		bufferSize = 0;
	}

	void LoadFont(const std::vector<const PrintInfo*>& prints);
	bool Loaded() { return (pixelData.pixels != nullptr); }

	void AddMoreCharacters(uint32_t count) { bufferSize += count; }

	int Descent() const { return descent; }
	int Ascent() const { return ascent; }

	//TODO: Shouldn't be in this class (Vulkan specific...)
	virtual void Draw(VulkanSystem& system, VkCommandBuffer commandBuffer) = 0;
	virtual PosData& GetNextPosData(VulkanSystem& system) = 0;

	bool Update(const std::vector<const PrintInfo*>& prints);

private:
	uint32_t CalculateTextureWidth();
	uint32_t CalculateTextureHeight(Packer& packer);
	void CreateFontTexture();
public:
	void InvalidateFont() { pixelData.TidyUp(); }
	bool AddCharacters(const std::u32string& text);

protected:
	uint32_t bufferSize = 0;
	std::string faceName;
	int fontSize;
	std::map<char32_t, std::unique_ptr<CharData>> charList;
	bool anyColourGlyphs;
	PixelData pixelData;
	std::map<char32_t, std::map<char32_t, int>> kerning;
	int descent;
	int ascent;
};

class Vulkan2DFont : public FontData
{
public:
	Vulkan2DFont(const std::string& faceName = "Arial", int fontSize = 24) : FontData(faceName, fontSize)
	{
	}

	PosData& GetNextPosData(VulkanSystem& system);
	uint32_t CurBufCharCount() const;

	Buffer vertexBuffer;
	void* vertexMemory = nullptr;
	PosData* pData = nullptr;

	virtual void Reset()
	{
		FontData::Reset();

		// Reset data pointer
		pData = (PosData*)vertexMemory;
	}

	void Setup(VulkanSystem& system, VulkanApplication& theApp, VkExtent2D workingExtent, const RenderPass& renderPass, int windowWidth, int windowHeight);
	void Draw(VulkanSystem& system, VkCommandBuffer commandBuffer);

	bool IsValid() const override { return pipeline.Created(); }

	void Tidy(VulkanSystem& system) override
	{
		vertexBuffer.DestroyBuffer(system);
	}

private:
	Descriptor descriptor;
	Pipeline pipeline;
	Texture texture;

	struct UboProj
	{
		glm::mat4 projection;
		int texWidth, texHeight;
	};
	UBO<UboProj> uboProj;
};

struct PrintInfo
{
	std::u32string text;
	glm::vec3 colour;
	float scale;
	FontData* font;
	std::vector<float> xVals;
};

class TypeSetter
{
public:
	void PrintStringInternal(FontData& fontData, const std::u32string& string, const glm::vec3& fontColour = glm::vec3(1.0f, 1.0f, 1.0f), float scale = 1.0f);

	static const size_t TYPESET_PADDING = 4;

	TypeSetter(size_t x = 0, size_t y = 0, size_t width = INVALID_SIZE, size_t height = INVALID_SIZE);
	~TypeSetter();

	void SetVerticalAlignment(VerticalAlignment val) { verticalAlignment = val; }
	void SetHorizontalAlignment(HorizontalAlignment val) { horizontalAlignment = val; }
	void Init(size_t x, size_t y, size_t width, size_t height);
	void Reset(bool clearPrints);

	bool PrintExpected(const PrintInfo& printInfo);
	bool OutOfDate() const { return stringCheckNum != prints.size(); }

	bool LayoutStrings(FontData* primaryFont);
	void Print(VulkanSystem& system, FontData* primaryFont);

	float MaxRowWidth() const { return maxRowWidth; }
	size_t NumRows() const;

	bool Visible() const { return maxWidth > 0 && maxHeight > 0; }
	float GetRowPos(size_t row) const;
	bool FirstPrint() const { return stringCheckNum == 0; }

	const std::vector<PrintInfo>& GetPrints() { return prints; }

	void SetBackground(const glm::vec3& colour) { backgroundColour = colour; }

protected:
	bool LayoutHorizontal(PrintInfo& print);
	void TruncatePreviousPrint();

	enum class AddPrintReturn { OKAY, RETRY_AT_START, PRINT_TOO_BIG };
	AddPrintReturn AddPrint(PrintInfo& print);
	void TrimPrints(size_t newSize);

private:
	size_t stringCheckNum;
	size_t xPos, yPos;
	HorizontalAlignment horizontalAlignment;
	VerticalAlignment verticalAlignment;
	float curXpos;
	size_t maxWidth, maxHeight;
	float maxRowWidth;
	bool truncated;
	glm::vec3 backgroundColour = glm::vec3(0.25);

	std::vector<PrintInfo> prints;
	std::unique_ptr<Heights> heights;
};
