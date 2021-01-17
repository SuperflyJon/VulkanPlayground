
#include "stdafx.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include "WinUtil.h"
#include "BitmapFont.h"
#include "BitmapFontInternal.h"

namespace FreeType
{
	class Delayed : public WinUtils::DelayedLib
	{
	public:
		Delayed() : WinUtils::DelayedLib("freetype.dll") {}

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
		void Delayed::Init()
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

	void CalcKern(std::map<char32_t, CharData>& charList, Kerning& kerning, FreeType::Delayed& ftLib, FT_Face face, char32_t firstChar, char32_t secondChar)
	{
		auto firstGlyphIndex = charList[firstChar].GetIndex();
		auto secondGlyphIndex = charList[secondChar].GetIndex();

		if (firstGlyphIndex != 0 && secondGlyphIndex != 0 && kerning.GetKerning(firstChar, secondChar) != 0)
		{
			FT_Vector kernAdvance;
			if (ftLib.FT_Get_Kerning(face, firstGlyphIndex, secondGlyphIndex, ft_kerning_unfitted, &kernAdvance) == 0 && kernAdvance.x != 0)
				kerning.SetKerning(firstChar, secondChar, kernAdvance.x);
		}
	}

	bool LoadGlyph(FreeType::Delayed& ftLib, FT_Face face, char32_t character, CharData& charData, bool& anyColourGlyphs)
	{
		if (charData.GetIndex() == 0 && character != '\n')
		{
			bool isTabChar = (character == U'\t');
			if (isTabChar)
				character = ' ';	// Use space and times advance by 4

			// Load character glyph
			int index = ftLib.FT_Get_Char_Index(face, character);
			if (index == 0)
				return false;
			//std::cout << "Loading character U+" << std::hex << std::uppercase << character << "\n";	// Debug output

			FT_Int32 loadFlags = FT_LOAD_RENDER;
			bool coloured = FT_HAS_COLOR(face);
			if (coloured)
			{
				loadFlags |= FT_LOAD_COLOR;
				anyColourGlyphs = true;
			}

			if (ftLib.FT_Load_Glyph(face, index, loadFlags))
				throw std::runtime_error("ERROR::FREETYTPE: Failed to load Glyph");

			auto advance = face->glyph->advance.x;
			if (isTabChar)
				advance *= 4;	// 4 spaces

			charData.Setup(index, advance, face->glyph->bitmap.width, face->glyph->bitmap.rows, face->glyph->bitmap_left, face->glyph->bitmap_top, coloured, face->glyph->bitmap.buffer);
		}
		return true;
	}

	bool LoadGlyphs(std::map<char32_t, CharData>& charList, FreeType::Delayed& ftLib, FT_Face face, bool incTofu, bool& anyColourGlyphs)
	{
		bool allFound = true;
		// Load data
		for (auto& ele : charList)
		{
			if (!LoadGlyph(ftLib, face, ele.first, ele.second, anyColourGlyphs))
			{
				if (!incTofu)
					allFound = false;	// Hopefully will be found in linked fonts...
				else
				{
					LoadGlyph(ftLib, face, 0x000025A1, ele.second, anyColourGlyphs);	// Tofu unfound character
					std::cout << "Unable to find charcter U+" << std::hex << std::uppercase << ele.first << "\n";
				}
			}
		}
		return allFound;
	}

	bool TryFont(std::map<char32_t, CharData>& charList, Kerning& kerning, const std::vector<std::u32string>& preloadStrs, FreeType::Delayed& ftLib, FT_Library ft, const std::string& fontFilename, int fontSize, bool incTofu, bool& anyColourGlyphs)
	{
		std::string filename{ fontFilename.c_str() };
		if (!WinUtils::FileExists(filename))
		{
			filename = WinUtils::GetWindowsFontPath() + filename;
			if (!WinUtils::FileExists(filename))
			{
				std::cout << "Unable to find font: " + fontFilename << "\n";
				return false;
			}
		}

		FT_Face face;
		if (ftLib.FT_New_Face(ft, filename.c_str(), 0, &face))
			throw std::runtime_error("ERROR::FREETYPE: Failed to load font");

		//ftLib.FT_Select_Charmap(face, FT_ENCODING_UNICODE);	// Not needed

		// Not sure this is needed - Set_Pixel_Sizes probably supercedes this
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
			ftLib.FT_Select_Size(face, best_match);
		}

		ftLib.FT_Set_Pixel_Sizes(face, 0, fontSize);

		bool allFound = LoadGlyphs(charList, ftLib, face, incTofu, anyColourGlyphs);

		// Get kerning values
		for (const auto& text : preloadStrs)
		{
			char32_t lastChar = '\0';
			for (auto character : text)
			{
				if (lastChar != '\0')
					CalcKern(charList, kerning, ftLib, face, lastChar, character);

				lastChar = character;
			}
		}
		ftLib.FT_Done_Face(face);
		return allFound;
	}

	void LoadFont(const std::string& faceName, std::map<char32_t, CharData>& charList, Kerning& kerning, const std::vector<std::u32string>& preloadStrs, int fontSize, bool& anyColourGlyphs)
	{
		auto fontFilename = WinUtils::GetFontFilename(faceName);

		static FreeType::Delayed ftLib;
		ftLib.Setup();

		FT_Library ft;
		if (ftLib.FT_Init_FreeType(&ft))
			throw std::runtime_error("ERROR::FREETYPE: Could not initialise FreeType Library");

		bool allFound = TryFont(charList, kerning, preloadStrs, ftLib, ft, fontFilename, fontSize, false, anyColourGlyphs);
		if (!allFound)
		{
			auto linkedFonts = WinUtils::GetLinkedFonts(faceName);
			if (linkedFonts.empty())
				linkedFonts = WinUtils::GetLinkedFonts("Lucida Sans Unicode");	// Try this font for fallbacks instead

			linkedFonts.insert(linkedFonts.begin(), WinUtils::GetFontFilename("Segoe UI Emoji"));	// Add colour emoji font as first fallback

			for (auto linkedFontFilename : linkedFonts)
			{
				allFound = TryFont(charList, kerning, preloadStrs, ftLib, ft, linkedFontFilename, fontSize, false, anyColourGlyphs);
				if (allFound)
					break;
			}
			if (!allFound)
				TryFont(charList, kerning, preloadStrs, ftLib, ft, fontFilename, fontSize, true, anyColourGlyphs);	// Tofu unknkown chars
		}

		ftLib.FT_Done_FreeType(ft);
	}
}
