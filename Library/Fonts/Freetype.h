#pragma once

typedef unsigned char byte;
typedef void * GlyphPtr;

class FreeTypeInternal;

class FreeTypeFont
{
public:
	struct GlyphData
	{
		GlyphData(unsigned int indexVal, long advanceVal, size_t widthVal, size_t rowsVal, int bitmap_leftVal, int bitmap_topVal, bool colouredVal, void* data);

		size_t width, rows;
		int bitmap_left, bitmap_top;
		size_t advance;
		bool coloured;
		int index;
		std::unique_ptr<byte[]> bitdata;
	};

	FreeTypeFont(const std::string& faceName, int fontSize);
	~FreeTypeFont();

	GlyphData LoadGlyphData(char32_t character, bool& anyColourGlyphs);

	int CalcKern(int firstChar, int secondChar);

	int GetLineAscent();
	int GetLineDescent();

protected:
	void FindFontNames(const std::string& faceName);
	void InitFreetype(int fontSize);
	GlyphPtr FindGlyphInFonts(char32_t character);
	GlyphPtr FindGlyph(char32_t character);

private:
	std::vector<std::string> fontNames;
	std::unique_ptr<FreeTypeInternal> m_internal;
};
