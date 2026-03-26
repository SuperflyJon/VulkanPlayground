
#include "Freetype.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <sstream>
#include <iomanip>
#include <map>
#include <array>
#include "../VulkanPlayground/WinUtil.h"

constexpr char32_t TofuChar = 0x000025A1;	// Tofu unfound character
constexpr char32_t UnicodeReplacementChar = 0xFFFD;	// Replacement character for invalid unicode sequences

std::string GetUnicodeOutput(char32_t character)
{
	std::ostringstream oss;
	oss << character;
	oss << "U+" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << character;
	return oss.str();
}

class FreetypeDelayed : public WinUtils::DelayedLib
{
public:
	FreetypeDelayed() : WinUtils::DelayedLib("freetype.dll") {}

	typedef FT_Error(*FT_Init_FreeTypeFP)(FT_Library* alibrary);
	typedef FT_Error(*FT_New_FaceFP)(FT_Library library, const char* filepathname, FT_Long face_index, FT_Face* aface);
	typedef FT_Error(*FT_Set_Pixel_SizesFP)(FT_Face face, FT_UInt pixel_width, FT_UInt pixel_height);
	typedef FT_Error(*FT_Load_GlyphFP)(FT_Face face, FT_ULong char_code, FT_Int32 load_flags);
	typedef FT_Error(*FT_Done_FaceFP)(FT_Face face);
	typedef FT_Error(*FT_Done_FreeTypeFP)(FT_Library library);

	typedef FT_Error(*FT_Select_CharmapFP)(FT_Face face, FT_Encoding encoding);
	typedef FT_Error(*FT_Select_SizeFP)(FT_Face face, FT_Int strike_index);
	typedef FT_UInt(*FT_Get_Char_IndexFP)(FT_Face face, FT_ULong charcode);
	typedef FT_Error(*FT_Get_KerningFP)(FT_Face face, FT_UInt left_glyph, FT_UInt right_glyph, FT_UInt kern_mode, FT_Vector* akerning);

	FT_Init_FreeTypeFP FT_Init_FreeType = nullptr;
	FT_New_FaceFP FT_New_Face = nullptr;
	FT_Set_Pixel_SizesFP FT_Set_Pixel_Sizes = nullptr;
	FT_Load_GlyphFP FT_Load_Glyph = nullptr;
	FT_Done_FreeTypeFP FT_Done_FreeType = nullptr;
	FT_Done_FaceFP FT_Done_Face = nullptr;
	FT_Select_CharmapFP FT_Select_Charmap = nullptr;
	FT_Select_SizeFP FT_Select_Size = nullptr;
	FT_Get_Char_IndexFP FT_Get_Char_Index = nullptr;
	FT_Get_KerningFP FT_Get_Kerning = nullptr;

protected:
	void FreetypeDelayed::Init()
	{
		LoadFunction(FT_Init_FreeType, "FT_Init_FreeType");
		LoadFunction(FT_New_Face, "FT_New_Face");
		LoadFunction(FT_Set_Pixel_Sizes, "FT_Set_Pixel_Sizes");
		LoadFunction(FT_Load_Glyph, "FT_Load_Glyph");
		LoadFunction(FT_Done_FreeType, "FT_Done_FreeType");
		LoadFunction(FT_Done_Face, "FT_Done_Face");
		LoadFunction(FT_Select_Charmap, "FT_Select_Charmap");
		LoadFunction(FT_Select_Size, "FT_Load_Glyph");
		LoadFunction(FT_Get_Char_Index, "FT_Get_Char_Index");
		LoadFunction(FT_Get_Kerning, "FT_Get_Kerning");
	}
};

FreetypeDelayed& GetFtLib()
{
	// Single instance to avoid re-loading freetype
	static FreetypeDelayed ftDll;
	return ftDll;
}

class FTI_Font
{
public:
	FTI_Font(FT_Library ftLib, const std::string& fontName, int fontSize)
	{
		if (GetFtLib().FT_New_Face(ftLib, fontName.c_str(), 0, &face))
			throw std::runtime_error("ERROR::FREETYPE: Failed to load font");

		// Fixed sized fonts are no longer used but just in case
		if (FT_HAS_FIXED_SIZES(face))
		{
			int best_match = 0;
			int diff = INT_MAX;
			for (int i = 0; i < face->num_fixed_sizes; ++i)
			{
				int ndiff = std::abs(fontSize - (face->available_sizes[i].x_ppem / 64));
				if (ndiff < diff)
				{
					best_match = i;
					diff = ndiff;
				}
			}
			GetFtLib().FT_Select_Size(face, best_match);
		}
		else
			GetFtLib().FT_Set_Pixel_Sizes(face, 0, fontSize);
	}
	~FTI_Font()
	{
		GetFtLib().FT_Done_Face(face);
	}
	
	FT_GlyphSlot FindGlyph(char32_t character)
	{
		if (character == U'\t')
			character = ' ';	// Use space and times advance by 4

		// Load character glyph
		int index = GetFtLib().FT_Get_Char_Index(face, character);
		if (index == 0)
			return nullptr;

		//std::cout << "Loading character " << GetUnicodeOutput(character) << "\n";	// Debug output

		FT_Int32 loadFlags = FT_LOAD_RENDER;
		if (FT_HAS_COLOR(face))
			loadFlags |= FT_LOAD_COLOR;

		if (GetFtLib().FT_Load_Glyph(face, index, loadFlags))
			throw std::runtime_error("ERROR::FREETYTPE: Failed to load Glyph");

		return face->glyph;
	}

	int GetKerning(int firstGlyphIndex, int secondGlyphIndex)
	{
		FT_Vector kernAdvance;
		if (GetFtLib().FT_Get_Kerning(face, firstGlyphIndex, secondGlyphIndex, ft_kerning_unfitted, &kernAdvance) == 0)
			return kernAdvance.x;
		else
			return 0;
	}

	int GetLineAscent() { return face->size->metrics.ascender; }
	int GetLineDescent() { return face->size->metrics.descender; }

private:
	FT_Face face;
};

class FreeTypeInternal
{
public:
	FreeTypeInternal(int fontSize)
		: fontSize(fontSize)
	{
		GetFtLib().Setup();

		if (GetFtLib().FT_Init_FreeType(&ftLib))
			throw std::runtime_error("ERROR::FREETYPE: Could not initialise FreeType Library");
	}
	~FreeTypeInternal()
	{
		fonts.clear();
		GetFtLib().FT_Done_FreeType(ftLib);
	}

	FTI_Font& CreateFont(const std::string& fontName)
	{
		auto it = fonts.find(fontName);
		if (it == fonts.end())
		{
			fonts[fontName] = std::make_unique<FTI_Font>(ftLib, fontName, fontSize);
			it = fonts.find(fontName);
		}
		return *it->second.get();
	}

	int GetKerning(const std::string& fontName, int firstGlyphIndex, int secondGlyphIndex)
	{
		return fonts[fontName]->GetKerning(firstGlyphIndex, secondGlyphIndex);
	}

	int GetLineAscent()
	{
		if (fonts.empty())
			return 0;
		else
			return fonts.begin()->second->GetLineAscent() / 64;
	}
	int GetLineDescent()
	{
		if (fonts.empty())
			return 0;
		else
			return fonts.begin()->second->GetLineDescent() / 64;
	}

private:
	FT_Library ftLib;

	std::map<std::string, std::unique_ptr<FTI_Font>> fonts;
	int fontSize;
};

FreeTypeFont::FreeTypeFont(const std::string& faceName, int fontSize)
{
	FindFontNames(faceName);
	InitFreetype(fontSize);
}

int FreeTypeFont::GetLineAscent()
{
	return m_internal->GetLineAscent();
}

int FreeTypeFont::GetLineDescent()
{
	return m_internal->GetLineDescent();
}

FreeTypeFont::~FreeTypeFont() = default;

void FreeTypeFont::FindFontNames(const std::string& faceName)
{
	auto AddFont = [this](std::string fontFilename)
		{
			if (!WinUtils::FileExists(fontFilename))
			{
				fontFilename = WinUtils::GetWindowsFontPath() + fontFilename;
				if (!WinUtils::FileExists(fontFilename))
					return;
			}
			fontNames.push_back(fontFilename);
		};

	AddFont(WinUtils::GetFontFilename(faceName));

	// Add colour emoji font as first fallback
	AddFont(WinUtils::GetFontFilename("Segoe UI Emoji"));

	// Find other fallback fonts
	auto linkedFonts = WinUtils::GetLinkedFonts(faceName);
	if (linkedFonts.empty())
		linkedFonts = WinUtils::GetLinkedFonts("Lucida Sans Unicode");	// Try this font for fallbacks instead

	for (auto linkedFont : linkedFonts)
		AddFont(linkedFont);
}

GlyphPtr FreeTypeFont::FindGlyphInFonts(char32_t character)
{
	for (auto& face : fontNames)
	{
		auto glyph = m_internal->CreateFont(face).FindGlyph(character);
		if (glyph != nullptr)
			return glyph;
	}
	return nullptr;
}

GlyphPtr FreeTypeFont::FindGlyph(char32_t character)
{
	if (auto glyph = FindGlyphInFonts(character))
		return glyph;

	// Not found - use a fallback character
	static constexpr std::array<char32_t, 3> fallbackChars { UnicodeReplacementChar, TofuChar, U'?' };
	for (auto fallbackChar : fallbackChars)
	{
		if (auto glyph = FindGlyphInFonts(fallbackChar))
			return glyph;
	}
	// Unlikely to get here but just in case - throw an error
	throw std::runtime_error("Unable to find glyph for character " + GetUnicodeOutput(character) + " and no more fallbacks available!");
}

void FreeTypeFont::InitFreetype(int fontSize)
{
	m_internal = std::make_unique<FreeTypeInternal>(fontSize);
}

std::unique_ptr<CharData> FreeTypeFont::LoadGlyphData(char32_t character, bool& anyColourGlyphs)
{
	auto glyph = (FT_GlyphSlot)FindGlyph(character);
	bool coloured = (glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA);
	if (coloured)
		anyColourGlyphs = true;

	auto advance = glyph->advance.x;
	if (character == U'\t')
		advance *= 4;	// 4 spaces

	return std::make_unique<CharData>(glyph->glyph_index, advance, glyph->bitmap.width, glyph->bitmap.rows, glyph->bitmap_left, glyph->bitmap_top, coloured, glyph->bitmap.buffer);
}

int FreeTypeFont::CalcKern(int firstChar, int secondChar)
{
	return m_internal->GetKerning(fontNames.front(), firstChar, secondChar);	// Use main font for kerning calculations
}
