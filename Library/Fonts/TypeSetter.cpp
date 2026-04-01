#include "pch.h"
#include "TypeSetter.h"
#include "BitmapFontInternal.h"
#include "CharData.h"

void TypeSetter::PrintStringInternal(FontData& fontData, const std::u32string& string, const glm::vec3& fontColour, float scale)
{
	PrintInfo printInfo{ string, fontColour, scale, &fontData, {} };
	if (!PrintExpected(printInfo))
	{
		prints.push_back(printInfo);
	}
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
		return (int)yPos - (int)heights->GetTotalHeight(TypeSetter::TYPESET_PADDING) + heights->GetRowOffset(row, TypeSetter::TYPESET_PADDING);
	else
		return (int)yPos + heights->GetRowOffset(row, TypeSetter::TYPESET_PADDING);
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

float ConvertPixelCount(size_t val)
{
	return (float)val / 64.0f;
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
		return (ConvertPixelCount(charData.Advance()) + elipses.LeftBearing()) * lastPrint.scale;
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
			float charStartPos = (ConvertPixelCount(xPixelCount + kerningAdjustment) + charData.LeftBearing()) * print.scale;

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
					maxRowWidth = std::max(maxRowWidth, curOffset + ConvertPixelCount(xPixelCount) * print.scale);
				}
			}
			startOfCurrentLine = it + 1;

			UpdateHeights();
			if (maxHeight != 0 && heights->GetTotalHeight(TypeSetter::TYPESET_PADDING) > maxHeight)
				break;	// exceeded max height

			heights->Add();

			charsAccountedFor = charNum;
			xPixelCount = 0;

			beginsAtLineStart = true;
		}

		previousChar = *it;
	}

	UpdateHeights();

	if (maxHeight != 0 && heights->GetTotalHeight(TypeSetter::TYPESET_PADDING) > maxHeight - TYPESET_PADDING
		&& !(it == print.text.end() && previousChar == '\n'))	// exceeded max height
	{
		print.text = std::u32string(print.text.begin(), it);
		return AddPrintReturn::PRINT_TOO_BIG;
	}

	maxRowWidth = std::max(maxRowWidth, (curXpos - startX) + endofCurChar);
	curXpos += ConvertPixelCount(xPixelCount) * print.scale;

	return AddPrintReturn::OKAY;
}

void TypeSetter::TypeSet()
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

	auto &firstFont = *prints.front().font;
	firstFont.SetupBackdrop(xPos + horizontalAdjustment, yPos, backdropWidth, heights->GetTotalHeight(TypeSetter::TYPESET_PADDING), backgroundColour, verticalAlignment);

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

				print.font->RecordCharacter(character, print.xVals[charNum] + horizontalAdjustment, rowPos, print.colour, print.scale);

				charNum++;
				overAllCharCount++;
			}
		}
	}
}

bool TypeSetter::LayoutStrings()
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

	if (stringCheckNum != INVALID_SIZE && stringCheckNum != prints.size() && !truncated)
		TrimPrints(stringCheckNum);

	if (!prints.empty())
	{
		heights->RemoveAnyEmptyLastRows();

		for (auto& print : prints)
			print.font->AddMoreSpace(CountNonWhiteSpace(print.text));

		prints.front().font->AddMoreSpace(1);	// For background character
	}

	return redraw;
}

void TypeSetter::TrimPrints(size_t newSize)
{
	prints.erase(prints.begin() + newSize, prints.end());
}
