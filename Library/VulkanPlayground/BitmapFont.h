#pragma once

#include "Descriptor.h"
#include "Pipeline.h"
#include "PixelData.h"
#include "Image.h"
#include "TextHelper.h"

class Packer;
class TypeSetter;
class CharData;
class Kerning;
class Heights;

enum class HorizontalAlignment { Left, Right };

class FontData
{
public:
	FontData(const std::string& _faceName, int _fontSize);
	~FontData();

	void ChangeFont(const std::string& faceName, int fontSize);

	void PreProcessString(const std::u32string& text);
	void PrintString(TypeSetter& typeSetter, const std::u32string& string, const glm::vec3 &fontColour, float scale);

	const CharData& GetChar(char32_t character) const;
	int GetKerning(char32_t first, char32_t second) const;

	TypeSetter& GetTypeSetter(int x, int y, unsigned int width, unsigned int height);
	void ClearTypeSetters(VulkanSystem& system);

	virtual bool IsValid() const = 0;
	bool UnexpectedChars() const { return unexpectedChars; }
	void Reset() { unexpectedChars = false; }

private:
	void LoadFont();
	uint32_t CalculateTextureWidth();
	uint32_t CalculateTextureHeight(Packer& packer);
	void CreateFontTexture();
	void InvalidateFont() { pixelData.TidyUp(); }
	bool AddCharacters(const std::u32string& text, bool showMissingChars = false);

protected:
	std::string faceName;
	int fontSize;
	std::map<char32_t, CharData> charList;
	bool anyColourGlyphs;
	std::vector<std::u32string> preloadStrs;
	std::vector<std::unique_ptr<TypeSetter>> typeSetters;
	PixelData pixelData;
	std::unique_ptr<Kerning> kerning;
	bool unexpectedChars = false;
};

class Vulkan2DFont : public FontData
{
public:
	Vulkan2DFont(const std::string& faceName = "Arial", int fontSize = 24) : FontData(faceName, fontSize)
	{}

	void Setup(VulkanSystem& system, VulkanApplication& theApp, VkExtent2D workingExtent, const RenderPass& renderPass, int windowWidth, int windowHeight);
	void Draw(VkCommandBuffer commandBuffer, Buffer& vertexBuffer, uint32_t numChars) const;

	bool IsValid() const override { return pipeline.Created(); }

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

class TypeSetter : public ITidy
{
public:
	static const size_t TYPESET_PADDING = 5;

	TypeSetter(FontData& fontData, size_t x = 0, size_t y = 0, size_t width = INVALID_SIZE, size_t height = INVALID_SIZE);
	~TypeSetter();
	void SetHorizontalAlignment(HorizontalAlignment val) { horizontalAlignment = val; }
	void Init(size_t x, size_t y, size_t width, size_t height, bool reset = true);
	void Reset(bool clear);
	void Clear() { prints.clear(); }
	void Tidy(VulkanSystem& system) override;
	void AddExtraBuffer(size_t amount) { extraBuffer = amount; }

	bool PrintExpected(const std::u32string& string, const glm::vec3& fontColour, float scale);
	void LayoutHorizontal(std::u32string text, const glm::vec3 &colour, float scale);
	bool LayoutVertical(VulkanSystem& system);
	void Draw(VulkanSystem& system, VkCommandBuffer commandBuffer);
	void UpdateBlankSpace(size_t pixels);
		
	float MaxRowWidth() const { return maxRowWidth; }
	size_t NumRows() const;
	size_t x() const { return xPos; }
	size_t y() const { return yPos; }
	bool Truncated() { return truncated; }
	bool Visible() const { return maxWidth > 0 && maxHeight > 0; }
	FontData& GetFont() const { return fontData; }
	float GetRowPos(size_t row) const;
	uint32_t CurBufCharCount() const;
	bool FirstPrint() const { return stringCheckNum == 0; }

	struct PrintInfo
	{
		std::u32string text;
		glm::vec3 colour;
		float scale;
		std::vector<float> xVals;
	};
	const std::vector<PrintInfo>& GetPrints() { return prints; }
	void* GetData() { return vertexMemory; }

	bool InUse() const { return inUse; }

protected:
	void TruncatePreviousPrint();

	enum class AddPrintReturn { OKAY, RETRY_AT_START, PRINT_TOO_BIG };
	AddPrintReturn AddPrint(std::u32string& text, float scale, float* xVals);

private:
	FontData& fontData;
	bool inUse, lastInUse;
	size_t stringCheckNum;
	size_t xPos, yPos;
	HorizontalAlignment horizontalAlignment;
	float curXpos;
	size_t maxWidth, maxHeight;
	float maxRowWidth;
	bool truncated;

	std::vector<PrintInfo> prints;
	std::unique_ptr<Heights> heights;

	size_t extraBuffer;
	Buffer vertexBuffer;
	void* vertexMemory;
};
