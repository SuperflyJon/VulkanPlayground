#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <glm/glm.hpp>

class CharData;
class FontData;
class FreeTypeFont;
class CharData;

enum class HorizontalAlignment { Left, Right };
enum class VerticalAlignment { Up, Down };

enum class YSwap { NoSwap, Swap };

struct PrintInfo
{
	std::u32string text;
	glm::vec3 colour;
	float scale;
	FontData* font;
	std::vector<float> xVals;
};

class FontData
{
public:
	FontData(const std::string& _faceName, int _fontSize);
	virtual ~FontData();

	const CharData& GetChar(char32_t character) const;
	int GetKerning(char32_t first, char32_t second) const;

	virtual bool IsValid() const { return true; }
	virtual void Reset()
	{
		bufferSize = 0;
	}

	void LoadFont(const std::vector<const PrintInfo*>& prints);
	bool Loaded() { return (bitmapData != nullptr); }

	bool AddCharacters(const std::u32string& text);

	void AddMoreSpace(uint32_t count) { bufferSize += count; }

	int Descent() const { return descent; }
	int Ascent() const { return ascent; }

	virtual void RecordCharacter(char32_t character, float x, float y, const glm::vec3& colour, float scale) = 0;
	virtual void SetupBackdrop(size_t x, size_t y, size_t backdropWidth, size_t backdropHeight, const glm::vec3& backgroundColour) = 0;

	bool Update(const std::vector<const PrintInfo*>& prints);

	virtual void SetupBuffers() {};

protected:
	void SetupKerning(FreeTypeFont& ft_font, const std::u32string& text);

	uint32_t bufferSize = 0;
	std::string faceName;
	int fontSize;
	std::map<char32_t, std::unique_ptr<CharData>> charList;
	bool anyColourGlyphs;
	std::unique_ptr<unsigned char> bitmapData;
	uint32_t bitmapWidth, bitmapHeight;
	std::map<char32_t, std::map<char32_t, int>> kerning;
	int descent;
	int ascent;
};
