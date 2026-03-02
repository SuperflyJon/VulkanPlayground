#pragma once

#include "Fonts/Freetype.h"

struct IsWhiteSpaceFn
{
	bool operator()(char32_t character) const { return character == ' ' || character == '\t' || character == '\n'; }
};
inline bool IsWhiteSpace(char32_t character) { return IsWhiteSpaceFn()(character); }
template<class iterator_type> uint32_t CountNonWhiteSpace(iterator_type begin, iterator_type end)
{
	return (uint32_t)std::count_if(begin, end, std::not_fn(IsWhiteSpaceFn()));
}
template<class string_type> uint32_t CountNonWhiteSpace(const string_type& str)
{
	return CountNonWhiteSpace(str.begin(), str.end());
}

struct PosData
{
	glm::vec2 pos, size;
	glm::uvec2 textorg, textoff;
	glm::vec3 colour;
	int coloured{ -1 };
};

class Heights
{
public:
	void Init();
	void Add();
	void ChangePreviousEmptyLineHeight(size_t pixels);
	void UpdateSpecialHeights(const Heights& oldHeights);
	void Update(float max, float min, float scale, size_t addedChars, bool includesSpecialChar);
	size_t GetTotalHeight() const;
	float GetRowOffset(size_t) const;
	size_t NumRows() const { return maxHeights.size(); }
	size_t GetCharsInRow(size_t row) const { return numCharsInLine[row]; }
	void RemoveAnyEmptyLastRows();
	void MakeHeightsUniform();

private:
	std::vector<float> maxHeights, minHeights, maxScales;
	std::vector<bool> hasSpecialChars;
	std::vector<size_t> numCharsInLine;
};

class CharData
{
public:
	CharData(FreeTypeFont::GlyphData&& glyphData)
		: glyphData(std::move(glyphData)), offsetX(0), offsetY(0)
	{
	}

	template<class TYPE> void CopyBitmapData(TYPE* destBuf, size_t X, size_t Y, size_t height, size_t curRowWidth);

	void Unload() { glyphData.index = 0; }
	bool Loaded() const { return glyphData.index != 0; }

	void SetIndex(int val) { glyphData.index = val; }
	int GetIndex() const { return glyphData.index; }

	int LeftBearing() const { return glyphData.bitmap_left; }
	int Bitmap_top() const { return glyphData.bitmap_top; }
	size_t Advance() const { return glyphData.advance; }
	size_t Width() const { return glyphData.width; }
	size_t Rows() const { return glyphData.rows; }
	size_t OffsetX() const { return offsetX; }
	size_t OffsetY() const { return offsetY; }
	bool Coloured() const { return glyphData.coloured; }

private:
	FreeTypeFont::GlyphData glyphData;
	size_t offsetX, offsetY;
};


class Packer
{
public:
	Packer()
	{
	}
	explicit Packer(size_t textureWidth, size_t edgePadding)
	{
		Setup(textureWidth, edgePadding);
	}
	void Setup(size_t textureWidth, size_t edgePadding)
	{
		rowWidth = textureWidth;
		padding = edgePadding;
		Reset();
		heights.clear();
	}
	void Reset()
	{
		curRowWidth = padding;
		curRowMaxHeight = 0;
		curRow = 0;
		curHeight = padding;
	}

	bool CalcPackSize(size_t width, size_t height);
	template<class TYPE> void Pack(CharData& charData, TYPE packBuffer);

	void NewRow(float scale = 1.0f)
	{
		curRowMaxHeight = (int)(curRowMaxHeight * scale);
		curHeight += curRowMaxHeight;
		heights.push_back(curRowMaxHeight);
		curRowMaxHeight = 0;
		curRow++;
	}

	size_t DoneGetTotalHeight()
	{
		NewRow();
		return curHeight;
	}

	size_t rowWidth;

	size_t curRow;
	size_t curRowWidth;
	size_t curRowMaxHeight;
	size_t curHeight;
	std::vector<size_t> heights;

	size_t padding;
};
