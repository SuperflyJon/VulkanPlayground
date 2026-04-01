#include "pch.h"
#include "FontData.h"
#include "Freetype.h"
#include "BitmapFontInternal.h"
#include <Utils/StrUtil.h>
#include <iostream>

FontData::FontData(const std::string& _faceName, int _fontSize, bool swapY) : faceName(_faceName), fontSize(_fontSize), anyColourGlyphs(false), swapY(swapY)
{
}
FontData::~FontData() = default;	// Other object files don't know how big FontData is...

const unsigned int FONT_BITMAP_SPACING = 1;

uint32_t CalculateTextureWidth(const std::map<char32_t, std::unique_ptr<CharData>>& charList)
{
	size_t totalGlyphWidth = 0, totalGlyphHeight = 0;
	for (auto& character : charList)
	{
		if (!IsWhiteSpace(character.first))
		{
			totalGlyphWidth += character.second->Width() + FONT_BITMAP_SPACING;
			totalGlyphHeight += character.second->Rows() + FONT_BITMAP_SPACING;
		}
	}

	// Get a rough width for font texture from totals
	auto roughWidth = (size_t)sqrt(totalGlyphWidth * (totalGlyphHeight / charList.size()));
	return (uint32_t)pow(2, ceil(log2(roughWidth)));	// Round to nearest power of 2
}

uint32_t CalculateTextureHeight(Packer& packer, const std::map<char32_t, std::unique_ptr<CharData>>& charList)
{
	// Work out height
	for (const auto& character : charList)
	{
		if (!IsWhiteSpace(character.first))
		{
			const CharData& charData = *character.second;
			packer.CalcPackSize(charData.Width() + FONT_BITMAP_SPACING, charData.Rows() + FONT_BITMAP_SPACING);
		}
	}
	auto height = packer.DoneGetTotalHeight();
	return (uint32_t)pow(2, ceil(log2(height)));	// Round to nearest power of 2
}

unsigned char* CreateFontTexture(const std::map<char32_t, std::unique_ptr<CharData>>& charList, uint32_t* width, uint32_t* height, bool anyColourGlyphs, bool swappedY)
{
	*width = CalculateTextureWidth(charList);
	if (width == 0)
		return nullptr;

	Packer packer(*width, FONT_BITMAP_SPACING, swappedY);
	*height = CalculateTextureHeight(packer, charList);
	auto numPixels = *width * *height;
	return packer.Pack(numPixels, charList, anyColourGlyphs);
}

void FontData::LoadFont(const std::vector<const PrintInfo*>& prints)
{
	AddCharacters(U".…");	// Ensure there is a dot as used for background and … if out of space

	FreeTypeFont ft_font(faceName, fontSize);

	for (auto &character : charList)
	{
		if (!character.second)
			character.second = ft_font.LoadGlyphData(character.first, anyColourGlyphs);
		std::cout << StrUtils::UTF32toUTF8({ character.first });
	}
	std::cout << "\n";

	ascent = ft_font.GetLineAscent();
	descent = ft_font.GetLineDescent();

	bitmapData.reset(CreateFontTexture(charList, &bitmapWidth, &bitmapHeight, anyColourGlyphs, swapY));

	for (auto& print : prints)
	{
		if (print->font == this)
			SetupKerning(ft_font, print->text);
	}
}

void FontData::SetupKerning(FreeTypeFont& ft_font, const std::u32string& text)
{	// Build up all possible transitions
	char32_t lastChar = '\0';
	for (auto character : text)
	{
		if (character == '\n')	// Skip new lines
			lastChar = '\0';
		else
		{
			if (lastChar != '\0')
			{
				int kern = ft_font.CalcKern(charList.at(lastChar)->GetIndex(), charList.at(character)->GetIndex());
				if (kern != 0)
					kerning[lastChar][character] = kern;
			}

			lastChar = character;
		}
	}
}

const CharData& FontData::GetChar(char32_t character) const
{
	auto it = charList.find(character);
	if (it == charList.end())
		throw std::runtime_error("Unexpected character not in font!");

	return *it->second;
}

int FontData::GetKerning(char32_t first, char32_t second) const
{
	if (auto firstPos = kerning.find(first); firstPos != kerning.end())
	{
		auto& secondMap = firstPos->second;
		if (auto secondPos = secondMap.find(second); secondPos != secondMap.end())
			return secondPos->second;
	}
	return 0;
}

bool FontData::AddCharacters(const std::u32string& text)
{
	bool anyNewChars = false;

	for (auto character : text)
	{
		if (character != '\n')
		{
			if (charList.find(character) == charList.end())
			{	// Add character to be loaded
				charList[character] = nullptr;
				anyNewChars = true;
			}
		}
	}

	return anyNewChars;
}

bool FontData::Update(const std::vector<const PrintInfo*>& prints)
{
	bool anyNewChars = false;
	for (auto& print : prints)
	{
		if (print->font == this)
		{
			if (AddCharacters(print->text))
				anyNewChars = true;
		}
	}

	if (anyNewChars)
		LoadFont(prints);	// Load font if any new characters

	return anyNewChars;
}
