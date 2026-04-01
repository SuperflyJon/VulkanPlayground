#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <glm/glm.hpp>

class CharData;
struct PixelData;

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

class FontData;
class TypeSetter;
extern std::set<FontData*> ProcessPrints(const std::set<TypeSetter*>& typeSetters, bool* redraw, bool* recreate);

class Heights
{
public:
	void Init();
	void Add();
	void UpdateSpecialHeights(const Heights& oldHeights);
	void Update(float max, float min, float scale, size_t addedChars, bool includesSpecialChar);
	size_t GetTotalHeight(size_t padding) const;
	float GetRowOffset(size_t row, size_t padding) const;
	size_t NumRows() const { return maxHeights.size(); }
	size_t GetCharsInRow(size_t row) const { return numCharsInLine[row]; }
	void RemoveAnyEmptyLastRows();

private:
	std::vector<float> maxHeights, minHeights, maxScales;
	std::vector<bool> hasSpecialChars;
	std::vector<size_t> numCharsInLine;
};

class Packer
{
public:
	explicit Packer(size_t textureWidth, size_t edgePadding, bool swappedY = false)
		: swappedY(swappedY)
	{
		Setup(textureWidth, edgePadding);
	}
	void Setup(size_t textureWidth, size_t edgePadding)
	{
		rowWidth = textureWidth;
		padding = edgePadding;
		Reset();
		packHeights.clear();
	}
	void Reset()
	{
		curRowWidth = padding;
		curRowMaxHeight = 0;
		curRow = 0;
		curHeight = padding;
	}

	bool CalcPackSize(size_t width, size_t height);

	unsigned char* Pack(size_t numPixels, const std::map<char32_t, std::unique_ptr<CharData>>& charList, bool colour);

	void NewRow(float scale = 1.0f)
	{
		curRowMaxHeight = (int)(curRowMaxHeight * scale);
		curHeight += curRowMaxHeight;
		packHeights.push_back(curRowMaxHeight);
		curRowMaxHeight = 0;
		curRow++;
	}

	size_t DoneGetTotalHeight()
	{
		NewRow();
		return curHeight;
	}

private:
	template<class TYPE> void Pack(CharData& charData, TYPE packBuffer);
	template<typename PIX> unsigned char* PackAll(size_t numPixels, const std::map<char32_t, std::unique_ptr<CharData>>& charList);

	bool swappedY;
	size_t rowWidth;

	size_t curRow;
	size_t curRowWidth;
	size_t curRowMaxHeight;
	size_t curHeight;
	std::vector<size_t> packHeights;

	size_t padding;
};
