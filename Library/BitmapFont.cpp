
#include "stdafx.h"
#include "BitmapFont.h"
#include "Application.h"
#include "SwapChain.h"
#include "System.h"
#include "WinUtil.h"
#include <numeric>
#include "BitmapFontInternal.h"

FontData::FontData(const std::string& _faceName, int _fontSize) : faceName(_faceName), fontSize(_fontSize), anyColourGlyphs(false)
{
}
FontData::~FontData() = default;	// Other object files don't know how big FontData is...

void FontData::LoadFont()
{
	AddCharacters(U".…");	// Ensure there is a dot as used for background and … if out of space

	FreeTypeFont ft_font(faceName, fontSize);
	std::cout << requiredChars.size() << " new chars: ";
	for (auto& character : requiredChars)
	{
		charList.emplace(character, ft_font.LoadGlyphData(character, anyColourGlyphs));
		std::cout << WinUtils::UTF32toUTF8({ character });
	}
	std::cout << "\n";

	CreateFontTexture();

	kerning.CalcKerning(ft_font, charList);
}

void FontData::SetupKerning(const std::u32string& text)
{	// Build up all possible transitions
	char32_t lastChar = '\0';
	for (auto character : text)
	{
		if (character == '\n')	// Skip new lines
			lastChar = '\0';
		else
		{
			if (lastChar != '\0')
				kerning.SetKerning(lastChar, character, 0);

			lastChar = character;
		}
	}
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
	return kerning.GetKerning(first, second);
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
	auto srcBuf = glyphData.bitdata.get();
	for (size_t i = 0; i < height; i++)
	{
		if (i < glyphData.rows)
		{
			for (size_t j = 0; j < glyphData.width; j++)
				destBuf[X + j].Copy(srcBuf, glyphData.coloured);

			memset(destBuf + (size_t)X + glyphData.width, 0, sizeof(TYPE));
		}

		destBuf += rowWidth;
	}
// Can't free this incase we need to recreate a larger bitmap with more characters...
//glyphData.bitdata.reset();	// Free bitmap data
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

	charData.CopyBitmapData(packBuffer + (size_t)rowWidth * curHeight, curRowWidth, curHeight, packHeights[curRow], rowWidth);
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
	if (pixelData.width == 0)
		return;

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
	kerning.Clear();
	for (auto& ele : charList)
		ele.second.Unload();
}

void Vulkan2DFont::Draw(VkCommandBuffer commandBuffer, Buffer &vertexBuffer, uint32_t numChars) const
{
	vertexBuffer.Bind(commandBuffer);
	pipeline.Bind(commandBuffer, descriptor);
	vkCmdDraw(commandBuffer, numChars, 1, 0, 0);
}

void TypeSetter::Draw(VulkanSystem& system, VkCommandBuffer commandBuffer)
{
	if (!prints.empty())
	{
		system.DebugInsertLabel(commandBuffer, WinUtils::UTF32toUTF8(prints[0].text), { 0.0f, 0.0f, 1.0f });
		((Vulkan2DFont&)fontData).Draw(commandBuffer, vertexBuffer, CurBufCharCount());
	}
}

void TypeSetter::PrintStringInternal(const std::u32string& string, const glm::vec3& fontColour, float scale)
{
	PrintInfo printInfo{ string, fontColour, scale, &fontData, {} };
// Todo pass in printInfo?
	if (!PrintExpected(printInfo.text, printInfo.colour, printInfo.scale))
	{
		if (fontData.AddCharacters(printInfo.text))
			fontData.InvalidateFont();

		prints.push_back(printInfo);
	}
}

void FontData::ClearTypeSetters(VulkanSystem& system)
{
	for (auto &typeSetter : typeSetters)
		typeSetter->Tidy(system);
	typeSetters.clear();
}

bool FontData::AddCharacters(const std::u32string& text)
{
	for (auto character : text)
	{
		if (character != '\n')
		{
			if (requiredChars.find(character) == requiredChars.end() && charList.find(character) == charList.end())
			{	// Add character to be loaded
				requiredChars.insert(character);
			}
		}
	}

	return !requiredChars.empty();
}

TypeSetter::TypeSetter(FontData& font, size_t x, size_t y, size_t width, size_t height)
	: fontData(font), heights(new Heights())
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

void TypeSetter::Init(size_t x, size_t y, size_t width, size_t height)
{
	xPos = x;
	yPos = y;
	maxWidth = std::max((int)width, 0);
	maxHeight = std::max((int)height, 0);
	maxRowWidth = 0;

	Reset();
}

void TypeSetter::Reset()
{
	truncated = false;
	prints.clear();
	stringCheckNum = 0;
}

void TypeSetter::Tidy(VulkanSystem& system)
{
	vertexBuffer.DestroyBuffer(system);
	Reset();
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

	// Remove unmatched prints
	prints.resize(stringCheckNum);
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

	auto& lastPrint = prints.back();

	// Tidy any white space
	heights->RemoveAnyEmptyLastRows();
	while (IsWhiteSpace(lastPrint.text.back()))
		lastPrint.text.pop_back();

	// Add elipses to previous string
	float* lastPrintChar = lastPrint.xVals.data() + CountNonWhiteSpace(lastPrint.text) - 1;
	const auto& elipses = fontData.GetChar(u'…');

	auto WorkoutElipsesPos = [&]()
	{
		auto& lastLine = lastPrint.text;
		if (lastLine.empty())
			return 0.f;

		auto& charData = fontData.GetChar(lastLine.back());
		return ConvertPixelCount(-charData.LeftBearing() + charData.Advance() + elipses.LeftBearing(), lastPrint.scale);
	};

	while (*lastPrintChar - xPos + WorkoutElipsesPos() + (elipses.Width() * lastPrint.scale) > maxWidth - TYPESET_PADDING)
	{
		lastPrint.text.pop_back();
		lastPrint.xVals.pop_back();
		if (lastPrint.xVals.empty())
			break;	// Give up if previous string doesn't fit (unlikely)
		lastPrintChar--;
	}

	// Add elipses
	auto elipsesXpos = *lastPrintChar + WorkoutElipsesPos();
	lastPrint.text.push_back(u'…');
	lastPrint.xVals.push_back(elipsesXpos);
	heights->Update(0, 0, 0, 1, false);	// One extra character

	maxRowWidth = std::max(maxRowWidth, (float)elipsesXpos - xPos + (elipses.Width() * lastPrint.scale) - TYPESET_PADDING);
}

bool TypeSetter::LayoutHorizontal(PrintInfo& print)
{
	if (maxWidth == 0 || maxHeight == 0)
		return true;	// Already out of space

	auto savedCurXpos = curXpos;
	auto savedHeights = *heights;

	print.xVals.resize(CountNonWhiteSpace(print.text));

	AddPrintReturn res;
	while ((res = AddPrint(print)) != AddPrintReturn::OKAY)
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
			if (!print.text.empty() && print.text.back() == u'…')
				print.text = print.text.substr(0, print.text.size() - 1);

			// Reduce string by one character
			print.text = print.text.substr(0, print.text.size() - 1);

			if (print.text.empty())
			{	// None of this string fits!
				return false;
			}

			if (print.text.back() == ' ')	// Skip spaces when finding break point
				print.text = print.text.substr(0, print.text.size() - 1);

			print.text += u'…';
			auto newSize = CountNonWhiteSpace(print.text);
			if (newSize > print.xVals.size())
			{	// Truncated at end of text (probably caused by whitespace)
				assert(print.xVals.size() == newSize - 1);
				print.xVals.resize(CountNonWhiteSpace(print.text));
			}
		}
	}
	if (truncated)
		print.xVals.resize(CountNonWhiteSpace(print.text));

	return true;
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

TypeSetter::AddPrintReturn TypeSetter::AddPrint(PrintInfo& print)
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

	if (curXpos == -1)
	{
		curXpos = startX;
	}

	bool beginsAtLineStart = (curXpos == startX);
	auto startOfCurrentLine = print.text.begin();

	auto UpdateHeights = [&]()
	{
		float curRowMaxHeight = 0;
		if (!maxCharHeights.empty())
			curRowMaxHeight = *std::max_element(maxCharHeights.begin(), maxCharHeights.end());
		float curRowMinHeight = 0;
		if (!minCharHeights.empty())
			curRowMinHeight = *std::min_element(minCharHeights.begin(), minCharHeights.end());

		heights->Update(curRowMaxHeight, curRowMinHeight, print.scale, charNum - charsAccountedFor, includesSpecialChar);
		heights->MakeHeightsUniform();

		includesSpecialChar = false;
	};

	auto it = print.text.begin();
	for (; it != print.text.end(); it++)
	{
		auto character = *it;
		bool newLineChar = (character == '\n');
		bool rowFull = false;

		if (IsWhiteSpace(character))
			lastBreakingPos = endofCurChar;

		if (!newLineChar)
		{
			const CharData& charData = fontData.GetChar(character);
			int kerningAdjustment = 0;
			if (xPixelCount > 0)
			{	// Not start of line
				kerningAdjustment = fontData.GetKerning(previousChar, character);
			}
			float charStartPos = ConvertPixelCount(xPixelCount + kerningAdjustment, print.scale) + (charData.LeftBearing() * print.scale);

			if (!IsWhiteSpace(character))
			{
				print.xVals[charNum] = curXpos + charStartPos;
				if (charData.Coloured())
					includesSpecialChar = true;
				charNum++;
			}

			endofCurChar = charStartPos + charData.Width() * print.scale;
			if ((curXpos - startX) + endofCurChar > maxWidth - TYPESET_PADDING * 2)
			{
				rowFull = true;
			}
			else
			{
				xPixelCount += kerningAdjustment + charData.Advance();
			}
			maxCharHeights.push_back(charData.Bitmap_top() * print.scale);
			minCharHeights.push_back((charData.Bitmap_top() - (int)charData.Rows()) * print.scale);
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
						breakIt = print.text.end();	// Skip if below
						break;
					}
				}
				if (breakIt == startOfCurrentLine)
				{
					if (!beginsAtLineStart && prints.size() > 0)
						return AddPrintReturn::RETRY_AT_START;	// Added word doesn't fit - break at start of previous text

					// Start of line, failed to find breaking point - break at previous char
					if (it == print.text.begin())
					{
						print.text.clear();
						break;
					}
					it--;
					charNum--;
					maxCharHeights.pop_back();
					minCharHeights.pop_back();
					maxRowWidth = std::max(maxRowWidth, curOffset + ConvertPixelCount(xPixelCount, print.scale));
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

	if (maxHeight != 0 && heights->GetTotalHeight() > maxHeight - TYPESET_PADDING
		&& !(it == print.text.end() && previousChar == '\n'))	// exceeded max height
	{
		print.text = std::u32string(print.text.begin(), it);
		return AddPrintReturn::PRINT_TOO_BIG;
	}

	maxRowWidth = std::max(maxRowWidth, (curXpos - startX) + endofCurChar);
	curXpos += ConvertPixelCount(xPixelCount, print.scale);

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
	bool redraw = stringCheckNum != prints.size();

	if (stringCheckNum != INVALID_SIZE)
		prints.resize(stringCheckNum);

	if (prints.empty())
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

bool TypeSetter::LayoutStrings(VulkanSystem& system)
{
	bool redraw = false;

	if (stringCheckNum != prints.size())
	{
		heights->Init();
		curXpos = -1;

		for (auto it = prints.begin(); it != prints.end(); it++)
		{
			// Setup xVals for each print
			bool truncatePrevious = !LayoutHorizontal(*it);
			if (truncatePrevious)
			{
				do
				{
					if (it == prints.begin())
						break;

					it--;
				} while (it->text.empty() || it->text == U"\n");
			}
			if (truncated)
				prints.resize(it - prints.begin() + 1);	// Remove any prints that don't fit

			if (truncatePrevious)
				TruncatePreviousPrint();
		}
		// Sort out vertical layout
		if (LayoutVertical(system))
			redraw = true;
	}
	return redraw;
}
