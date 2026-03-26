
#include "stdafx.h"
#include "BitmapFont.h"
#include "Application.h"
#include "SwapChain.h"
#include "System.h"
#include "WinUtil.h"
#include <numeric>
#include "BitmapFontInternal.h"
#include "Fonts/Freetype.h"

FontData::FontData(const std::string& _faceName, int _fontSize) : faceName(_faceName), fontSize(_fontSize), anyColourGlyphs(false)
{
}
FontData::~FontData() = default;	// Other object files don't know how big FontData is...

uint32_t Vulkan2DFont::CurBufCharCount() const
{
	return (uint32_t)(vertexBuffer.GetBufferSize() / sizeof(PosData));
}

PosData& Vulkan2DFont::GetNextPosData(VulkanSystem& system)
{
	if (!vertexBuffer.Created() || bufferSize != CurBufCharCount())
	{
		if (vertexBuffer.Created())
			system.DeviceWaitIdle();	// Probably in use in command buffer

		vertexBuffer.Create(system, sizeof(PosData) * bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "vertexData");
		vertexMemory = vertexBuffer.Map(system.GetDevice());
		pData = (PosData*)vertexMemory;

		if (bufferSize < CurBufCharCount())
		{	// Zero off any extra unused characters
			for (size_t extra = bufferSize; extra < CurBufCharCount(); extra++)
			{
				pData[extra].size = glm::vec2();
				bufferSize++;
			}
		}
	}
	if (pData - (PosData*)vertexMemory >= bufferSize)
		throw std::runtime_error("Buffer overrun accessing print data");

	return *pData++;
}

void FontData::LoadFont(const std::vector<const PrintInfo*>& prints)
{
	AddCharacters(U".…");	// Ensure there is a dot as used for background and … if out of space

	FreeTypeFont ft_font(faceName, fontSize);

	for (auto &character : charList)
	{
		if (!character.second)
			character.second = ft_font.LoadGlyphData(character.first, anyColourGlyphs);
		std::cout << WinUtils::UTF32toUTF8({ character.first });
	}
	std::cout << "\n";

	ascent = ft_font.GetLineAscent();
	descent = ft_font.GetLineDescent();

	CreateFontTexture();

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
	auto srcBuf = bitdata.get();
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
			totalGlyphWidth += character.second->Width() + FONT_BITMAP_SPACING;
			totalGlyphHeight += character.second->Rows() + FONT_BITMAP_SPACING;
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
			const CharData& charData = *character.second;
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
				packer.Pack(*character.second, (RGBA*)pixelData.pixels);
		}
	}
	else
	{
		pixelData.pixels = (byte*)calloc((size_t)pixelData.width * pixelData.height, sizeof(Grey));
		for (auto& character : charList)
		{
			if (!IsWhiteSpace(character.first))
				packer.Pack(*character.second, (Grey*)pixelData.pixels);
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

void Vulkan2DFont::Draw(VulkanSystem& system, VkCommandBuffer commandBuffer)
{
	if (bufferSize == 0)
		return;	// Nothing to draw in this font

	system.DebugInsertLabel(commandBuffer, faceName, { 0.0f, 0.0f, 1.0f });

	vertexBuffer.Bind(commandBuffer);
	pipeline.Bind(commandBuffer, descriptor);
	vkCmdDraw(commandBuffer, bufferSize, 1, 0, 0);
}

void TypeSetter::PrintStringInternal(FontData& fontData, const std::u32string& string, const glm::vec3& fontColour, float scale)
{
	PrintInfo printInfo{ string, fontColour, scale, &fontData, {} };
	if (!PrintExpected(printInfo))
	{
		prints.push_back(printInfo);
	}
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

TypeSetter::TypeSetter(size_t x, size_t y, size_t width, size_t height)
	: heights(new Heights())
{
	horizontalAlignment = HorizontalAlignment::Left;
	verticalAlignment = VerticalAlignment::Up;
	Init(x, y, width, height);
}
TypeSetter::~TypeSetter() = default;

size_t TypeSetter::NumRows() const
{
	return heights->NumRows();
}
float TypeSetter::GetRowPos(size_t row) const
{
	if (verticalAlignment == VerticalAlignment::Up)
		return (int)yPos - (int)heights->GetTotalHeight() + heights->GetRowOffset(row);
	else
		return (int)yPos + heights->GetRowOffset(row);
}

void TypeSetter::Init(size_t x, size_t y, size_t width, size_t height)
{
	xPos = x;
	yPos = y;
	maxWidth = std::max((int)width, 0);
	maxHeight = std::max((int)height, 0);
	maxRowWidth = 0;

	Reset(true);
}

void TypeSetter::Reset(bool clearPrints)
{
	truncated = false;
	stringCheckNum = 0;

	if (clearPrints)
		prints.clear();
}

float ConvertPixelCount(size_t val, float scale)
{
	return ((float)val / 64.0f) * scale;
}

void SetupBackdrop(PosData& data, const CharData& charData, size_t x, size_t y, size_t backdropWidth, size_t backdropHeight, const glm::vec3 &backgroundColour, VerticalAlignment verticalAlignment)
{
	data.pos = glm::vec2((float)x, (verticalAlignment == VerticalAlignment::Up) ? (float)y - backdropHeight : (float)y);
	data.size = glm::vec2(backdropWidth, backdropHeight);
	// Make a rougth background effect
	data.textorg = glm::uvec2((short)charData.OffsetX() + (charData.Width() + 1) / 2, (short)charData.OffsetY());
	data.textoff = glm::uvec2(1, 1);
	data.colour = backgroundColour;
	data.coloured = charData.Coloured() ? 1 : 0;
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

bool TypeSetter::PrintExpected(const PrintInfo& printInfo)
{
	if (stringCheckNum == INVALID_SIZE)
		return false;

	if (stringCheckNum < prints.size())
	{
		auto& checkPrint = prints[stringCheckNum];
		if ((checkPrint.colour == printInfo.colour) && (checkPrint.scale == printInfo.scale) && (checkPrint.font == printInfo.font))
		{
			if (checkPrint.text == printInfo.text)
			{
				stringCheckNum++;
				return true;
			}
			if ((stringCheckNum == prints.size() - 1) && (checkPrint.text.back() == u'…') &&
				(printInfo.text.size() >= checkPrint.text.size() - 1) && (checkPrint.text == printInfo.text.substr(0, checkPrint.text.size() - 1) + U"…"))
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
	TrimPrints(stringCheckNum);
	stringCheckNum = INVALID_SIZE;	// Difference in prints
	return false;
}

void TypeSetter::TruncatePreviousPrint()
{
	if (prints.size() == 0)
		return;

	if (prints.size() == 1)
	{	// There is no previous print...
		TrimPrints(0);
		return;
	}

	auto& lastPrint = prints.back();

	// Tidy any white space
	heights->RemoveAnyEmptyLastRows();
	while (IsWhiteSpace(lastPrint.text.back()))
		lastPrint.text.pop_back();

	// Add elipses to previous string
	float* lastPrintChar = lastPrint.xVals.data() + CountNonWhiteSpace(lastPrint.text) - 1;
	const auto& elipses = lastPrint.font->GetChar(u'…');

	auto WorkoutElipsesPos = [&]()
	{
		auto& lastLine = lastPrint.text;
		if (lastLine.empty())
			return 0.f;

		auto& charData = lastPrint.font->GetChar(lastLine.back());
		return ConvertPixelCount(charData.LeftBearing() + charData.Advance() + elipses.LeftBearing(), lastPrint.scale);
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
	while (!numCharsInLine.empty() && numCharsInLine.back() == 0)
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

		offset += latestRowHeight;
	}
	return offset;
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

	if (curXpos == -1)
	{
		curXpos = startX;
	}

	bool beginsAtLineStart = (curXpos == startX);
	auto startOfCurrentLine = print.text.begin();

	auto UpdateHeights = [&]()
	{
		float curRowMaxHeight = print.font->Ascent() * print.scale;
		float curRowMinHeight = print.font->Descent() * print.scale;
		heights->Update(curRowMaxHeight, curRowMinHeight, print.scale, charNum - charsAccountedFor, includesSpecialChar);
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
			const CharData& charData = print.font->GetChar(character);
			int kerningAdjustment = 0;
			if (xPixelCount > 0)
			{	// Not start of line
				kerningAdjustment = print.font->GetKerning(previousChar, character);
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
					maxRowWidth = std::max(maxRowWidth, curOffset + ConvertPixelCount(xPixelCount, print.scale));
				}
			}
			startOfCurrentLine = it + 1;

			UpdateHeights();
			if (maxHeight != 0 && heights->GetTotalHeight() > maxHeight)
				break;	// exceeded max height

			heights->Add();

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

void TypeSetter::Print(VulkanSystem& system, FontData* primaryFont)
{
	if (prints.empty())
		return;

	// Work out background size
	size_t backdropWidth = (size_t)maxRowWidth + TYPESET_PADDING * 2;
	if (maxWidth != INVALID_SIZE && backdropWidth > maxWidth)
		backdropWidth = maxWidth;

	size_t horizontalAdjustment = 0;
	if (horizontalAlignment == HorizontalAlignment::Right)
		horizontalAdjustment = maxWidth - backdropWidth;

	SetupBackdrop(primaryFont->GetNextPosData(system), primaryFont->GetChar('.'), xPos + horizontalAdjustment, yPos, backdropWidth, heights->GetTotalHeight(), backgroundColour, verticalAlignment);

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

				SetData(print.font->GetNextPosData(system), print.font->GetChar(character), print.xVals[charNum] + horizontalAdjustment, rowPos, print.colour, print.scale);

				charNum++;
				overAllCharCount++;
			}
		}
	}
}

bool TypeSetter::LayoutStrings(FontData* primaryFont)
{
	heights->Init();
	curXpos = -1;
	truncated = false;

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
			TrimPrints(it - prints.begin() + 1);	// Remove any prints that don't fit

		if (truncatePrevious)
			TruncatePreviousPrint();

		if (prints.empty())
			break;	// Nothing fit...
	}

	bool redraw = OutOfDate();

	if (stringCheckNum != INVALID_SIZE)
		TrimPrints(stringCheckNum);

	if (!prints.empty())
	{
		heights->RemoveAnyEmptyLastRows();

		for (auto& print : prints)
			print.font->AddMoreCharacters(CountNonWhiteSpace(print.text));

		primaryFont->AddMoreCharacters(1);	// For background character
	}

	return redraw;
}

void TypeSetter::TrimPrints(size_t newSize)
{
	prints.erase(prints.begin() + newSize, prints.end());
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
