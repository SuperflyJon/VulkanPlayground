#pragma once

#include "CharData.h"

typedef void* GlyphPtr;

class FreeTypeInternal;

class FreeTypeFont
{
public:
	FreeTypeFont(const std::string& faceName, int fontSize);
	~FreeTypeFont();

	std::unique_ptr<CharData> LoadGlyphData(char32_t character, bool& anyColourGlyphs);

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
