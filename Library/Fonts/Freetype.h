#pragma once

typedef unsigned char byte;
typedef void * GlyphPtr;

class FreeTypeInternal;

class Kerning
{
public:
	void SetKerning(char32_t first, char32_t second, int kern) { kerning[first][second] = kern; }
	int GetKerning(char32_t first, char32_t second) const;
	void Clear() { kerning.clear(); }

private:
	std::map<char32_t, std::map<char32_t, int>> kerning;
};

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

	void CalcKern(Kerning& kerning, char32_t firstChar, int firstGlyphIndex, char32_t secondChar, int secondGlyphIndex);

protected:
	void FindFontNames(const std::string& faceName);
	void InitFreetype(int fontSize);
	GlyphPtr FindGlyphInFonts(char32_t character);
	GlyphPtr FindGlyph(char32_t character);

private:
	std::vector<std::string> fontNames;
	std::unique_ptr<FreeTypeInternal> m_internal;
};
