
#pragma once

#include <memory>
#include <string>
#include <vector>

typedef unsigned char byte;

class CharData
{
public:
	CharData(unsigned int indexVal, long advanceVal, size_t widthVal, size_t rowsVal, int bitmap_leftVal, int bitmap_topVal, bool colouredVal, void* data)
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
	bool Coloured() const { return coloured; }

	size_t OffsetX() const { return offsetX; }
	size_t OffsetY() const { return offsetY; }

	void SetOffsetX(size_t val) { offsetX = val; }
	void SetOffsetY(size_t val) { offsetY = val; }

	void ResetBitData() { bitdata.reset(); }
	byte* GetBitData() const { return bitdata.get(); }

private:
	size_t width, rows;
	int bitmap_left, bitmap_top;
	size_t advance;
	bool coloured;
	int index;
	std::unique_ptr<byte[]> bitdata;

	size_t offsetX, offsetY;
};
