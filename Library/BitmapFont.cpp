
#include "stdafx.h"
#include "BitmapFont.h"
#include "Application.h"
#include "SwapChain.h"
#include "System.h"
#include "WinUtil.h"
#include <numeric>
#include "BitmapFontInternal.h"

FontData::FontData(const std::string& _faceName, int _fontSize) : faceName(_faceName), fontSize(_fontSize), anyColourGlyphs(false), kerning(new Kerning)
{
}
FontData::~FontData() = default;	// Other object files don't know how big FontData is...

namespace FreeType
{
	void LoadFont(const std::string& faceName, std::map<char32_t, CharData>& charList, Kerning& kerning, const std::vector<std::u32string>& preloadStrs, int fontSize, bool& anyColourGlyphs);
}

void FontData::LoadFont()
{
	AddCharacters(U".…");	// Ensure there is a dot as used for background and … if out of space

	FreeType::LoadFont(faceName, charList, *kerning, preloadStrs, fontSize, anyColourGlyphs);

	CreateFontTexture();
}

const CharData& FontData::GetChar(char32_t character) const
{
	auto it = charList.find(character);
	if (it == charList.end())
		throw std::runtime_error("Unexpected character not in font!");

	return it->second;
}

int FontData::GetKerning(char32_t first, char32_t second) const
{
	return kerning->GetKerning(first, second);
}

TypeSetter& FontData::GetTypeSetter(int x, int y, unsigned int width, unsigned int height)
{
	for (auto &typeSetter : typeSetters)
	{
		if (!typeSetter->InUse())
		{
			typeSetter->Init(x, y, width, height, false);
			return *typeSetter;
		}
	}
	typeSetters.emplace_back(new TypeSetter(*this, x, y, width, height));
	return *typeSetters.back();
}

void CharData::Setup(int indexVal, size_t advanceVal, size_t widthVal, size_t rowsVal, int bitmap_leftVal, int bitmap_topVal, bool colouredVal, void* data)
{
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
	void Copy(byte*& srcBuf, bool coloured)
	{
		if (coloured)
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

template<class TYPE>
void CharData::CopyBitmapData(TYPE* destBuf, size_t X, size_t Y, size_t height, size_t rowWidth)
{
	offsetX = X;
	offsetY = Y;

	// Copy glyph
	byte* srcBuf = bitdata.get();
	for (size_t i = 0; i < height; i++)
	{
		if (i < rows)
		{
			for (size_t j = 0; j < width; j++)
				destBuf[X + j].Copy(srcBuf, coloured);

			memset(destBuf + (size_t)X + width, 0, sizeof(TYPE));
		}

		destBuf += rowWidth;
	}
}

template<class TYPE>
void Packer::Pack(CharData& charData, TYPE packBuffer)
{
	size_t width = charData.Width() + padding;

	if (curRowWidth + width > rowWidth)
	{
		curHeight += heights[curRow];
		curRow++;
		curRowWidth = padding;
	}

	charData.CopyBitmapData(packBuffer + (size_t)rowWidth * curHeight, curRowWidth, curHeight, heights[curRow], rowWidth);
	curRowWidth += width;
}

const unsigned int FONT_BITMAP_SPACING = 1;

uint32_t FontData::CalculateTextureWidth()
{
	size_t totalGlyphWidth = 0, totalGlyphHeight = 0;
	for (auto& character : charList)
	{
		if (!IsWhiteSpace(character.first))
		{
			totalGlyphWidth += character.second.Width() + FONT_BITMAP_SPACING;
			totalGlyphHeight += character.second.Rows() + FONT_BITMAP_SPACING;
		}
	}

	// Get a rough width for font texture from totals
	auto roughWidth = (size_t)sqrt(totalGlyphWidth * (totalGlyphHeight / charList.size()));
	return (uint32_t)pow(2, ceil(log2(roughWidth)));	// Round to nearest power of 2
}

uint32_t FontData::CalculateTextureHeight(Packer& packer)
{
	// Work out height
	for (const auto& character : charList)
	{
		if (!IsWhiteSpace(character.first))
		{
			const CharData& charData = character.second;
			packer.CalcPackSize(charData.Width() + FONT_BITMAP_SPACING, charData.Rows() + FONT_BITMAP_SPACING);
		}
	}
	auto height = packer.DoneGetTotalHeight();
	return (uint32_t)pow(2, ceil(log2(height)));	// Round to nearest power of 2
}

void FontData::CreateFontTexture()
{
	pixelData.width = CalculateTextureWidth();
	Packer packer(pixelData.width, FONT_BITMAP_SPACING);
	pixelData.height = CalculateTextureHeight(packer);
	packer.Reset();

	if (anyColourGlyphs)
	{
		pixelData.pixels = (byte*)calloc((size_t)pixelData.width * pixelData.height, sizeof(RGBA));
		for (auto& character : charList)
		{
			if (!IsWhiteSpace(character.first))
				packer.Pack(character.second, (RGBA*)pixelData.pixels);
		}
	}
	else
	{
		pixelData.pixels = (byte*)calloc((size_t)pixelData.width * pixelData.height, sizeof(Grey));
		for (auto& character : charList)
		{
			if (!IsWhiteSpace(character.first))
				packer.Pack(character.second, (Grey*)pixelData.pixels);
		}
	}
}

int Kerning::GetKerning(char32_t first, char32_t second) const
{
	auto firstPos = kerning.find(first);
	if (firstPos != kerning.end())
	{
		auto& secondMap = firstPos->second;
		auto secondPos = secondMap.find(second);
		if (secondPos != secondMap.end())
			return secondPos->second;
	}
	return 0;
}

void Vulkan2DFont::Setup(VulkanSystem& system, VulkanApplication& theApp, VkExtent2D workingExtent, const RenderPass& renderPass, int windowWidth, int windowHeight)
{
	if (!IsValid())
	{
		texture.Create2dTexture(system, pixelData, anyColourGlyphs ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8_UNORM, faceName + " Font Texture");

		descriptor.AddUniformBuffer(system, 0, uboProj, "Projection", VK_SHADER_STAGE_GEOMETRY_BIT);
		descriptor.AddTexture(2, texture);
		theApp.CreateDescriptor(system, descriptor, "Font");

		std::vector<Attribs::Attrib> attribs{
			{0, Attribs::Type::Misc, VK_FORMAT_R32G32_SFLOAT},	//position
			{1, Attribs::Type::Misc, VK_FORMAT_R32G32_SFLOAT},	//size
			{2, Attribs::Type::Misc, VK_FORMAT_R32G32_UINT},	//textorg
			{3, Attribs::Type::Misc, VK_FORMAT_R32G32_UINT},	//textoff
			{4, Attribs::Type::Misc, VK_FORMAT_R32G32B32_SFLOAT},	//colour
			{5, Attribs::Type::Misc, VK_FORMAT_R32_SINT}	//coloured
		};
		pipeline.SetupVertexDescription(attribs);
		pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
		pipeline.LoadShaderWithGeomShader(system, "Font", "");
		pipeline.EnableBlending();
		pipeline.SetDepthTestEnabled(false);
		theApp.CreatePipeline(system, renderPass, pipeline, descriptor, workingExtent, "Font");

		uboProj().projection = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);
		uboProj().texWidth = pixelData.width;
		uboProj().texHeight = pixelData.height;
		uboProj.CopyToDevice(system);
	}
}

void FontData::ChangeFont(const std::string& _faceName, int _fontSize)
{
	faceName = _faceName;
	fontSize = _fontSize;

	// Reload characters
	InvalidateFont();
	kerning->Clear();
	for (auto& ele : charList)
		ele.second.Unload();
}

void Vulkan2DFont::Draw(VkCommandBuffer commandBuffer, Buffer &vertexBuffer, uint32_t numChars) const
{
	vertexBuffer.Bind(commandBuffer);
	pipeline.Bind(commandBuffer, descriptor);
	vkCmdDraw(commandBuffer, numChars, 1, 0, 0);
}

void TypeSetter::Reset(bool clear)
{
	lastInUse = inUse;
	inUse = false;
	fontData.Reset();
	if (clear)
	{
		prints.clear();
		stringCheckNum = 0;
	}
}

void TypeSetter::Draw(VulkanSystem& system, VkCommandBuffer commandBuffer)
{
	if (inUse && !prints.empty())
	{
		system.DebugInsertLabel(commandBuffer, WinUtils::UTF32toUTF8(prints[0].text), { 0.0f, 0.0f, 1.0f });
		((Vulkan2DFont&)fontData).Draw(commandBuffer, vertexBuffer, CurBufCharCount());
	}
}

void FontData::PrintString(TypeSetter& typeSetter, const std::u32string& string, const glm::vec3 &fontColour, float scale)
{
	if (!typeSetter.PrintExpected(string, fontColour, scale))
	{
		if (AddCharacters(string, true))
		{	// Printing something with unexpected characters
			InvalidateFont();
			if (IsValid())
			{
				unexpectedChars = true;
				return;	// Need to recreate font
			}
		}
		
		// Ensure font is loaded
		if (pixelData.pixels == nullptr)
			LoadFont();

		typeSetter.LayoutHorizontal(string, fontColour, scale);
	}
}

void FontData::ClearTypeSetters(VulkanSystem& system)
{
	for (auto &typeSetter : typeSetters)
		typeSetter->Tidy(system);
	typeSetters.clear();
}

bool FontData::AddCharacters(const std::u32string& text, bool showMissingChars)
{
	std::u32string newChars;
	for (auto character : text)
	{
		if ((charList.find(character) == charList.end()) && (character != '\n'))
		{
			// Add character to be loaded
			charList.emplace(character, CharData());
			newChars.push_back(character);
		}
	}
	if (showMissingChars && !newChars.empty())
		std::cout << "Unexpcted characters: " << WinUtils::UTF32toUTF8(newChars) << std::endl;

	return !newChars.empty();
}

void FontData::PreProcessString(const std::u32string& text)
{
	if (std::find(preloadStrs.begin(), preloadStrs.end(), text) == preloadStrs.end())
	{
		AddCharacters(text);
		preloadStrs.push_back(text);
	}
}

TypeSetter::TypeSetter(FontData& font, size_t x, size_t y, size_t width, size_t height)
	: fontData(font), heights(new Heights()), extraBuffer(0)
{
	vertexMemory = nullptr;
	horizontalAlignment = HorizontalAlignment::Left;
	Init(x, y, width, height);
}
TypeSetter::~TypeSetter() = default;

size_t TypeSetter::NumRows() const
{
	return heights->NumRows();
}
float TypeSetter::GetRowPos(size_t row) const
{
	return (int)yPos - (int)heights->GetTotalHeight() + heights->GetRowOffset(row);
}

uint32_t TypeSetter::CurBufCharCount() const
{
	return (uint32_t)(vertexBuffer.GetBufferSize() / sizeof(PosData));
}

void TypeSetter::Init(size_t x, size_t y, size_t width, size_t height, bool reset)
{
	xPos = x;
	yPos = y;
	maxWidth = std::max((int)width, 0);
	maxHeight = std::max((int)height, 0);
	maxRowWidth = 0;
	if (reset)
	{
		inUse = false;
		lastInUse = false;
		truncated = false;
		prints.clear();
		stringCheckNum = 0;
	}
}

void TypeSetter::Tidy(VulkanSystem& system)
{
	vertexBuffer.DestroyBuffer(system);
}

float ConvertPixelCount(size_t val, float scale)
{
	return ((float)val / 64.0f) * scale;
}

void SetData(PosData& data, const CharData& charData, float x, float y, const glm::vec3 &colour, float scale)
{
	data.pos.x = x;
	data.pos.y = y - (charData.Bitmap_top() * scale);
	data.size = glm::vec2(charData.Width(), charData.Rows()) * scale;
	data.textorg = glm::uvec2((short)charData.OffsetX(), (short)charData.OffsetY());
	data.textoff = glm::uvec2((short)charData.Width(), (short)charData.Rows());
	data.colour = colour;
	data.coloured = charData.Coloured() ? 1 : 0;
}

bool TypeSetter::PrintExpected(const std::u32string& string, const glm::vec3 &fontColour, float scale)
{
	inUse = true;	// Print is active

	if (stringCheckNum == INVALID_SIZE)
		return false;

	if (stringCheckNum < prints.size())
	{
		auto& checkPrint = prints[stringCheckNum];
		if ((checkPrint.colour == fontColour) && (checkPrint.scale == scale))
		{
			if (checkPrint.text == string)
			{
				stringCheckNum++;
				return true;
			}
			if ((stringCheckNum == prints.size() - 1) && (checkPrint.text.back() == u'…') && 
				(string.size() >= checkPrint.text.size() - 1) && (checkPrint.text == string.substr(0, checkPrint.text.size() - 1) + U"…"))
			{
				stringCheckNum++;
				truncated = true;
				return true;
			}
		}
	}
	if ((stringCheckNum == prints.size()) && truncated)
		return true;

	// Remove any future prints and replay any that matched
	auto oldHeights = *heights;
	prints.resize(stringCheckNum);
	std::vector<PrintInfo> previousPrints;
	previousPrints.swap(prints);
	for (auto& previousPrint : previousPrints)
		LayoutHorizontal(previousPrint.text, previousPrint.colour, previousPrint.scale);
	heights->UpdateSpecialHeights(oldHeights);

	stringCheckNum = INVALID_SIZE;	// Difference in prints
	return false;
}

void TypeSetter::UpdateBlankSpace(size_t pixels)
{
	heights->ChangePreviousEmptyLineHeight(pixels);
}

void TypeSetter::TruncatePreviousPrint()
{
	if (prints.empty())
		return;

	// Tidy any white space
	heights->RemoveAnyEmptyLastRows();
	while (IsWhiteSpace(prints.back().text.back()))
		prints.back().text.pop_back();

	// Add elipses to previous string
	float* lastPrintChar = prints.back().xVals.data() + CountNonWhiteSpace(prints.back().text) - 1;
	const auto& elipses = fontData.GetChar(u'…');

	auto WorkoutElipsesPos = [&]()
	{
		auto& lastLine = prints.back().text;
		if (lastLine.empty())
			return 0.f;

		auto& charData = fontData.GetChar(lastLine.back());
		return ConvertPixelCount(-charData.LeftBearing() + charData.Advance() + elipses.LeftBearing(), prints.back().scale);
	};

	while (*lastPrintChar - xPos + WorkoutElipsesPos() + (elipses.Width() * prints.back().scale) > maxWidth - TYPESET_PADDING)
	{
		prints.back().text.pop_back();
		prints.back().xVals.pop_back();
		if (prints.back().xVals.empty())
			break;	// Give up if previous string doesn't fit (unlikely)
		lastPrintChar--;
	}

	// Add elipses
	auto elipsesXpos = *lastPrintChar + WorkoutElipsesPos();
	prints.back().text.push_back(u'…');
	prints.back().xVals.push_back(elipsesXpos);
	heights->Update(0, 0, 0, 1, false);	// One extra character

	maxRowWidth = std::max(maxRowWidth, (float)elipsesXpos + (elipses.Width() * prints.back().scale) - TYPESET_PADDING);
}

void TypeSetter::LayoutHorizontal(std::u32string text, const glm::vec3 &colour, float scale)
{
	if (truncated || maxWidth == 0 || maxHeight == 0)
		return;	// Already out of space

	auto savedCurXpos = curXpos;
	auto savedHeights = *heights;

	std::vector<float> xVals;
	xVals.resize(CountNonWhiteSpace(text));

	AddPrintReturn res;
	while ((res = AddPrint(text, scale, xVals.data())) != AddPrintReturn::OKAY)
	{
		if (res == AddPrintReturn::RETRY_AT_START)
		{
			savedHeights.Add();
			curXpos = (float)(xPos + TYPESET_PADDING);
			savedCurXpos = curXpos;
		}

		*heights = savedHeights;

		if (res == AddPrintReturn::PRINT_TOO_BIG)
		{	// Add elipses to shortend string (retry until it fits)
			truncated = true;
			curXpos = savedCurXpos;
			// Remove previous elipses (if present)
			if (!text.empty() && text.back() == u'…')
				text = text.substr(0, text.size() - 1);

			// Reduce string by one character
			text = text.substr(0, text.size() - 1);

			if (text.empty())
			{	// None of this string fits!
				TruncatePreviousPrint();
				return;
			}

			if (text.back() == ' ')	// Skip spaces when finding break point
				text = text.substr(0, text.size() - 1);

			text += u'…';
			auto newSize = CountNonWhiteSpace(text);
			if (newSize > xVals.size())
			{	// Truncated at end of text (probably caused by whitespace)
				assert(xVals.size() == newSize - 1);
				xVals.resize(CountNonWhiteSpace(text));
			}
		}
	}
	if (truncated)
		xVals.resize(CountNonWhiteSpace(text));

	if (!text.empty())
		prints.push_back({ text, colour, scale, xVals});
}

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

void Heights::ChangePreviousEmptyLineHeight(size_t pixels)
{
	if (NumRows() > 1)
	{
		auto previousLine = NumRows() - 2;
		if (numCharsInLine[previousLine] == 0)
		{
			maxHeights[previousLine] = (float)pixels;
			hasSpecialChars[previousLine] = true;
		}
	}
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
	while (numCharsInLine.back() == 0)
	{
		minHeights.pop_back();
		maxHeights.pop_back();
		maxScales.pop_back();
		hasSpecialChars.pop_back();
		numCharsInLine.pop_back();
	}
}

size_t Heights::GetTotalHeight() const
{
	return (size_t)GetRowOffset(NumRows() - 1) + (size_t)-minHeights[NumRows() - 1] + TypeSetter::TYPESET_PADDING;	// Position of last row + last minHeight and bottom padding
}

float Heights::GetRowOffset(size_t row) const
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

	float offset = TypeSetter::TYPESET_PADDING + firstHeight;
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

 		offset += latestRowHeight + TypeSetter::TYPESET_PADDING;
	}
	return offset;
}

void Heights::MakeHeightsUniform()
{
	if (NumRows() < 2)
		return;

	size_t firstLine = NumRows() - 1;
	while (firstLine > 0 && maxScales[firstLine - 1] == maxScales[firstLine])
		firstLine--;

	if (firstLine == NumRows() - 1)
		return;

	float lowestMin = 0;
	float largestMax = 0;

	for (size_t row = firstLine; row < NumRows(); row++)
	{
		if (!hasSpecialChars[row])
		{
			lowestMin = std::min(lowestMin, minHeights[row]);
			largestMax = std::max(largestMax, maxHeights[row]);
		}
	}
	for (size_t row = firstLine; row < NumRows(); row++)
	{
		if (!hasSpecialChars[row])
		{
			minHeights[row] = std::min(lowestMin, minHeights[row]);
			maxHeights[row] = std::max(largestMax, maxHeights[row]);
		}
	}
}

TypeSetter::AddPrintReturn TypeSetter::AddPrint(std::u32string &text, float scale, float* xVals)
{
	char32_t previousChar = '\0';
	float lastBreakingPos = 0;
	float endofCurChar = 0;
	size_t charNum = 0;
	size_t charsAccountedFor = 0;
	size_t xPixelCount = 0;
	bool includesSpecialChar = false;
	float startX = (float)(xPos + TYPESET_PADDING);

	std::vector<float> maxCharHeights, minCharHeights;

	if (prints.empty())
	{
		heights->Init();
		curXpos = startX;
	}

	bool beginsAtLineStart = (curXpos == startX);
	auto startOfCurrentLine = text.begin();

	auto UpdateHeights = [&]()
	{
		float curRowMaxHeight = 0;
		if (!maxCharHeights.empty())
			curRowMaxHeight = *std::max_element(maxCharHeights.begin(), maxCharHeights.end());
		float curRowMinHeight = 0;
		if (!minCharHeights.empty())
			curRowMinHeight = *std::min_element(minCharHeights.begin(), minCharHeights.end());

		heights->Update(curRowMaxHeight, curRowMinHeight, scale, charNum - charsAccountedFor, includesSpecialChar);
		heights->MakeHeightsUniform();

		includesSpecialChar = false;
	};

	auto it = text.begin();
	for (; it != text.end(); it++)
	{
		auto character = *it;
		bool newLineChar = (character == '\n');
		bool rowFull = false;

		if (IsWhiteSpace(character))
			lastBreakingPos = endofCurChar;

		if (!newLineChar)
		{
			const CharData& charData = fontData.GetChar(character);
			int kerning = 0;
			if (xPixelCount > 0)
			{	// Not start of line
				kerning = fontData.GetKerning(previousChar, character);
			}
			float charStartPos = ConvertPixelCount(xPixelCount + kerning, scale) + (charData.LeftBearing() * scale);

			if (!IsWhiteSpace(character))
			{
				xVals[charNum] = curXpos + charStartPos;
				if (charData.Coloured())
					includesSpecialChar = true;
				charNum++;
			}

			endofCurChar = charStartPos + charData.Width() * scale;
			if ((curXpos - startX) + endofCurChar > maxWidth - TYPESET_PADDING * 2)
			{
				rowFull = true;
			}
			else
			{
				xPixelCount += kerning + charData.Advance();
			}
			maxCharHeights.push_back(charData.Bitmap_top() * scale);
			minCharHeights.push_back((charData.Bitmap_top() - (int)charData.Rows()) * scale);
		}

		if (newLineChar || rowFull)
		{
			if (maxHeight == 0 && !newLineChar)	// Single line and exceeded max width (allow new lines)
				break;

			auto curOffset = (curXpos - startX);
			maxRowWidth = std::max(maxRowWidth, curOffset + lastBreakingPos);
			curXpos = startX;

			if (!IsWhiteSpace(character))
			{	// Need to find previous breaking character and start from there on new line
				auto breakIt = it;
				auto breakNum = charNum - 1;
				while (breakIt != startOfCurrentLine)
				{
					breakIt--;
					breakNum--;

					if (*breakIt == ' ' || *breakIt == '\t')
					{	// Break here
						auto removed = it - breakIt;
						maxCharHeights.resize(maxCharHeights.size() - removed);
						minCharHeights.resize(minCharHeights.size() - removed);

						charNum -= removed;
						it = breakIt;
						breakIt = text.end();	// Skip if below
						break;
					}
				}
				if (breakIt == startOfCurrentLine)
				{
					if (!beginsAtLineStart && prints.size() > 0)
						return AddPrintReturn::RETRY_AT_START;	// Added word doesn't fit - break at start of previous text

					// Start of line, failed to find breaking point - break at previous char
					if (it == text.begin())
					{
						text.clear();
						break;
					}
					it--;
					charNum--;
					maxCharHeights.pop_back();
					minCharHeights.pop_back();
					maxRowWidth = std::max(maxRowWidth, curOffset + ConvertPixelCount(xPixelCount, scale));
				}
			}
			startOfCurrentLine = it + 1;

			UpdateHeights();
			if (maxHeight != 0 && heights->GetTotalHeight() > maxHeight)
				break;	// exceeded max height

			heights->Add();

			maxCharHeights.clear();
			minCharHeights.clear();

			charsAccountedFor = charNum;
			xPixelCount = 0;

			beginsAtLineStart = true;
		}

		previousChar = *it;
	}

	UpdateHeights();

	if (maxHeight != 0 && heights->GetTotalHeight() > maxHeight)	// exceeded max height
	{
		text = std::u32string(text.begin(), it);
		return AddPrintReturn::PRINT_TOO_BIG;
	}

	maxRowWidth = std::max(maxRowWidth, (curXpos - startX) + endofCurChar);
	curXpos += ConvertPixelCount(xPixelCount, scale);
	xPixelCount = 0;

	return AddPrintReturn::OKAY;
}

void SetupBackdrop(PosData& data, const CharData& charData, size_t x, size_t y, size_t backdropWidth, size_t backdropHeight)
{
	data.pos = glm::vec2((float)x, (float)y - backdropHeight);
	data.size = glm::vec2(backdropWidth, backdropHeight);
	// Make a rougth background effect
	data.textorg = glm::uvec2((short)charData.OffsetX() + (charData.Width() + 1) / 2, (short)charData.OffsetY());
	data.textoff = glm::uvec2(1, 1);
	data.colour = glm::vec3(0.25, 0.25, 0.25);
	data.coloured = charData.Coloured() ? 1 : 0;
}

bool TypeSetter::LayoutVertical(VulkanSystem& system)
{
	truncated = false;
	bool redraw = (inUse != lastInUse);
	bool noChange = stringCheckNum == prints.size();
	stringCheckNum = 0;
	if (!inUse || prints.empty() || noChange)
		return redraw;	// Redraw if visibility changed

	heights->RemoveAnyEmptyLastRows();

	size_t bufferSize = 1;	// For background character
	for (auto& print : prints)
		bufferSize += CountNonWhiteSpace(print.text);

	if (!vertexBuffer.Created() || bufferSize > CurBufCharCount())
	{
		redraw = true;
		if (vertexBuffer.Created())
			system.DeviceWaitIdle();	// Probably in use in command buffer
		else
			bufferSize += extraBuffer;
		vertexBuffer.Create(system, sizeof(PosData) * bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "vertexData");
		vertexMemory = vertexBuffer.Map(system.GetDevice());
	}
	PosData* pData = (PosData*)vertexMemory;

	while (bufferSize < CurBufCharCount())
	{
		pData[bufferSize].size = glm::vec2();	// Zero off any extra unused characters
		bufferSize++;
	}

	// Work out background size
	size_t backdropWidth = (size_t)maxRowWidth + TYPESET_PADDING * 2;
	if (maxWidth != INVALID_SIZE && backdropWidth > maxWidth)
		backdropWidth = maxWidth;

	size_t horizontalAdjustment = 0;
	if (horizontalAlignment == HorizontalAlignment::Right)
		horizontalAdjustment = maxWidth - backdropWidth;

	SetupBackdrop(pData[0], fontData.GetChar('.'), xPos + horizontalAdjustment, yPos, backdropWidth, heights->GetTotalHeight());

	size_t curRow = 0;
	size_t endOfCurrentRow = INVALID_SIZE;
	size_t lastRow = INVALID_SIZE;
	float rowPos = 0;
	size_t overAllCharCount = 0;
	// Work out the y pos for each line
	for (auto& print : prints)
	{
		size_t charNum = 0;
		for (auto character : print.text)
		{
			if (!IsWhiteSpace(character))
			{
				pData++;

				if (overAllCharCount == endOfCurrentRow)
					curRow++;

				if ((int)curRow != lastRow)
				{
					if (lastRow == INVALID_SIZE)
						endOfCurrentRow = 0;

					while (curRow + 1 < heights->NumRows() && heights->GetCharsInRow(curRow) == 0)
						curRow++;
					if (curRow >= heights->NumRows())
						break;	// Probably super narrow

					endOfCurrentRow += heights->GetCharsInRow(curRow);

					rowPos = GetRowPos(curRow);
					lastRow = curRow;
				}

				SetData(*pData, fontData.GetChar(character), print.xVals[charNum] + horizontalAdjustment, rowPos, print.colour, print.scale);

				charNum++;
				overAllCharCount++;
			}
		}
	}

	return redraw;
}
