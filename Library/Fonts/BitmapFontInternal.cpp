#include "pch.h"
#include "BitmapFontInternal.h"
#include "CharData.h"
#include <assert.h>

void Heights::Init()
{
	minHeights.clear();
	maxHeights.clear();
	maxScales.clear();
	hasSpecialChars.clear();
	numCharsInLine.clear();
	Add();
}

void Heights::Add()
{
	minHeights.push_back(0);
	maxHeights.push_back(0);
	maxScales.push_back(0);
	hasSpecialChars.push_back(false);
	numCharsInLine.push_back(0);
}

void Heights::UpdateSpecialHeights(const Heights& oldHeights)
{
	for (int i = 0; i < oldHeights.NumRows() && i < NumRows(); i++)
	{
		if (oldHeights.hasSpecialChars[i])
		{
			hasSpecialChars[i] = true;
			maxHeights[i] = oldHeights.maxHeights[i];
			minHeights[i] = oldHeights.minHeights[i];
		}
	}
}

void Heights::Update(float max, float min, float scale, size_t addedChars, bool includesSpecialChar)
{
	maxHeights.back() = std::max(maxHeights.back(), max);
	minHeights.back() = std::min(minHeights.back(), min);
	maxScales.back() = std::max(maxScales.back(), scale);
	hasSpecialChars.back() = hasSpecialChars.back() || includesSpecialChar;
	numCharsInLine.back() += addedChars;
}

void Heights::RemoveAnyEmptyLastRows()
{
	while (!numCharsInLine.empty() && numCharsInLine.back() == 0)
	{
		minHeights.pop_back();
		maxHeights.pop_back();
		maxScales.pop_back();
		hasSpecialChars.pop_back();
		numCharsInLine.pop_back();
	}
}

size_t Heights::GetTotalHeight(size_t padding) const
{
	return (size_t)GetRowOffset(NumRows() - 1, padding) + (size_t)-minHeights[NumRows() - 1] + padding;	// Position of last row + last minHeight and bottom padding
}

float Heights::GetRowOffset(size_t row, size_t padding) const
{
	assert(row < maxHeights.size());

	size_t curRow = 0;
	float firstHeight = maxHeights.front();
	if (firstHeight == 0)
	{	// Add empty line(s) based on first non-empty line size
		while (curRow < NumRows() - 1)
		{
			curRow++;
			if (maxHeights[curRow] > 0)
			{
				firstHeight = (maxHeights[curRow] + -maxHeights[curRow]) * curRow;
				break;
			}
		}
	}

	float offset = padding + firstHeight;
	float latestRowHeight = firstHeight;
	while (curRow < row)
	{
		curRow++;
		if (maxHeights[curRow] > 0)
		{
			latestRowHeight = maxHeights[curRow];
			offset += -minHeights[curRow - 1];
		}
		else if (curRow == row)
			latestRowHeight = 0;

		offset += latestRowHeight;
	}
	return offset;
}

CharData::CharData(unsigned int indexVal, long advanceVal, size_t widthVal, size_t rowsVal, int bitmap_leftVal, int bitmap_topVal, bool colouredVal, void* data)
{
	offsetX = 0;
	offsetY = 0;

	index = indexVal;
	advance = advanceVal;
	width = widthVal;
	rows = rowsVal;
	bitmap_left = bitmap_leftVal;
	bitmap_top = bitmap_topVal;
	coloured = colouredVal;

	size_t dataSize = (size_t)width * rows;
	if (coloured)
		dataSize *= 4;	// rgba data
	bitdata = std::make_unique<byte[]>(dataSize);
	memcpy(bitdata.get(), data, dataSize);
}

template<class TYPE>
void CharData::CopyBitmapData(TYPE* destBuf, size_t X, size_t Y, size_t height, size_t rowWidth, bool swappedY)
{
	offsetX = X;
	offsetY = Y;

	// Copy glyph
	auto srcBuf = bitdata.get();

	if (swappedY)
		destBuf += rowWidth * rows;

	for (size_t i = 0; i < height; i++)
	{
		if (i < rows)
		{
			for (size_t j = 0; j < width; j++)
				destBuf[X + j].Copy(srcBuf, coloured);

			memset(destBuf + (size_t)X + width, 0, sizeof(TYPE));
		}

		if (swappedY)
			destBuf -= rowWidth;
		else
			destBuf += rowWidth;
	}
}

template<class TYPE>
void Packer::Pack(CharData& charData, TYPE packBuffer)
{
	size_t width = charData.Width() + padding;

	if (curRowWidth + width > rowWidth)
	{
		curHeight += packHeights[curRow];
		curRow++;
		curRowWidth = padding;
	}

	charData.CopyBitmapData(packBuffer + (size_t)rowWidth * curHeight, curRowWidth, curHeight, packHeights[curRow], rowWidth, swappedY);
	curRowWidth += width;
}

bool Packer::CalcPackSize(size_t width, size_t height)
{
	bool rowFull = curRowWidth + width > rowWidth - padding;
	if (rowFull)
	{
		NewRow();
		curRowWidth = padding;
	}
	if (height > curRowMaxHeight)
	{
		curRowMaxHeight = height;
	}

	curRowWidth += width;
	return rowFull;
}

template<typename PIX>
unsigned char* Packer::PackAll(size_t numPixels, const std::map<char32_t, std::unique_ptr<CharData>> &charList)
{
	unsigned char* pixelData = (byte*)calloc(numPixels, sizeof(PIX));
	for (auto& character : charList)
	{
		if (!IsWhiteSpace(character.first))
			Pack(*character.second, (PIX*)pixelData);
	}
	return pixelData;
}

struct Grey
{
	void Copy(byte*& srcBuf, bool /*coloured*/)
	{
		grey = *srcBuf++;
	}
	unsigned char grey;
};

struct RGBA
{
	void Copy(byte*& srcBuf, bool rgbColours)
	{
		if (rgbColours)
		{
			blue = *srcBuf++;
			green = *srcBuf++;
			red = *srcBuf++;
			alpha = *srcBuf++;
		}
		else
		{	// Just grey scale in red channel
			red = *srcBuf++;
		}
	}
	unsigned char red, green, blue, alpha;
};

unsigned char* Packer::Pack(size_t numPixels, const std::map<char32_t, std::unique_ptr<CharData>>& charList, bool colour)
{
	Reset();

	if (colour)
		return PackAll<RGBA>(numPixels, charList);
	else
		return PackAll<Grey>(numPixels, charList);
}

#include "FontData.h"
#include "TypeSetter.h"
std::set<FontData*> ProcessPrints(const std::set<TypeSetter*>& typeSetters, bool* redraw, bool* recreate)
{
	*redraw = false;
	*recreate = false;

	std::set<FontData*> currentFonts;
	std::vector<const PrintInfo*> outofdatePrints;
	for (auto checkTS : typeSetters)
	{
		if (checkTS->OutOfDate())
		{
			for (auto& print : checkTS->GetPrints())
			{
				outofdatePrints.push_back(&print);
				currentFonts.insert(print.font);
			}
		}
	}
	if (outofdatePrints.empty())
		return currentFonts;

	for (auto font : currentFonts)
	{
		if (font->Update(outofdatePrints))
			*recreate = true;
	}

	for (auto typeSetter : typeSetters)
	{
		if (typeSetter->LayoutStrings())
			*redraw = true;
	}

	for (auto font : currentFonts)
	{
		font->SetupBuffers();
	}

	for (auto typeSetter : typeSetters)
	{
		typeSetter->TypeSet();
	}

	return currentFonts;
}
