#pragma once

#include "FontData.h"

class FontData;
class Heights;

const size_t INVALID_SIZE = (size_t)-1;

class TypeSetter
{
public:
	void PrintStringInternal(FontData& fontData, const std::u32string& string, const glm::vec3& fontColour = glm::vec3(1.0f, 1.0f, 1.0f), float scale = 1.0f);

	static const size_t TYPESET_PADDING = 4;

	TypeSetter(size_t x = 0, size_t y = 0, size_t width = INVALID_SIZE, size_t height = INVALID_SIZE);
	~TypeSetter();

	void SetVerticalAlignment(VerticalAlignment val) { verticalAlignment = val; }
	void SetHorizontalAlignment(HorizontalAlignment val) { horizontalAlignment = val; }
	void Init(size_t x, size_t y, size_t width, size_t height);
	void Reset(bool clearPrints);

	bool PrintExpected(const PrintInfo& printInfo);
	bool OutOfDate() const { return stringCheckNum != prints.size(); }

	bool LayoutStrings();
	void TypeSet();

	float MaxRowWidth() const { return maxRowWidth; }
	size_t NumRows() const;

	bool Visible() const { return maxWidth > 0 && maxHeight > 0; }
	float GetRowPos(size_t row) const;
	bool FirstPrint() const { return stringCheckNum == 0; }

	const std::vector<PrintInfo>& GetPrints() { return prints; }

	void SetBackground(const glm::vec3& colour) { backgroundColour = colour; }

protected:
	bool LayoutHorizontal(PrintInfo& print);
	void TruncatePreviousPrint();

	enum class AddPrintReturn { OKAY, RETRY_AT_START, PRINT_TOO_BIG };
	AddPrintReturn AddPrint(PrintInfo& print);
	void TrimPrints(size_t newSize);

private:
	size_t stringCheckNum;
	size_t xPos, yPos;
	HorizontalAlignment horizontalAlignment;
	VerticalAlignment verticalAlignment;
	float curXpos;
	size_t maxWidth, maxHeight;
	float maxRowWidth;
	bool truncated;
	glm::vec3 backgroundColour = glm::vec3(0.25);

	std::vector<PrintInfo> prints;
	std::unique_ptr<Heights> heights;
};
