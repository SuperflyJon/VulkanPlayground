#pragma once

typedef unsigned char byte;

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

class Kerning
{
public:
	void SetKerning(char32_t first, char32_t second, int kern) { kerning[first][second] = kern; }
	int GetKerning(char32_t first, char32_t second) const;
	void Clear() { kerning.clear(); }

private:
	std::map<char32_t, std::map<char32_t, int>> kerning;
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
	CharData() : width(0), rows(0), bitmap_left(0), bitmap_top(0), advance(0), offsetX(0), offsetY(0), coloured(false), index(0)
	{ }

	void Setup(int indexVal, size_t advanceVal, size_t widthVal, size_t rowsVal, int bitmap_leftVal, int bitmap_topVal, bool colouredVal, void* data);
	template<class TYPE> void CopyBitmapData(TYPE* destBuf, size_t X, size_t Y, size_t height, size_t curRowWidth);

	void Unload() { index = 0; }
	bool Loaded() const { return index != 0; }

	void SetIndex(int val) { index = val; }
	int GetIndex() const { return index; }

	int LeftBearing() const { return bitmap_left; }
	int Bitmap_top() const { return bitmap_top; }
	size_t Advance() const { return advance; }
	size_t Width() const { return width; }
	size_t Rows() const { return rows; }
	size_t OffsetX() const { return offsetX; }
	size_t OffsetY() const { return offsetY; }
	bool Coloured() const { return coloured; }

private:
	size_t width, rows;
	int bitmap_left, bitmap_top;
	size_t advance;
	size_t offsetX, offsetY;
	bool coloured;
	int index;
	std::unique_ptr<byte[]> bitdata;
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
