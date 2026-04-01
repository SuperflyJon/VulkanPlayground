#include "pch.h"
#include "ConUtil.h"
#include <Windows.h>
#include <iostream>

void OutputWithConsoleAttributes(const std::string& text, WORD attributes, bool newLineAtEnd = true, std::ostream& stream = std::cout)
{
	CONSOLE_SCREEN_BUFFER_INFO csbiOrgInfo;
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hStdout, &csbiOrgInfo);

	SetConsoleTextAttribute(hStdout, attributes);
	stream << text;
	SetConsoleTextAttribute(hStdout, csbiOrgInfo.wAttributes);
	if (newLineAtEnd)
		std::cout << std::endl;
}

void ConUtils::EnableUnicodeConsoleOutput()
{
	SetConsoleOutputCP(CP_UTF8);
}

void ConUtils::OutputError(const std::string& text)
{
	OutputWithConsoleAttributes(text, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_INTENSITY, true, std::cerr);
}

void ConUtils::OutputWarning(const std::string& text)
{
	OutputWithConsoleAttributes(text, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_RED | BACKGROUND_INTENSITY, true, std::cerr);
}

void ConUtils::OutputBlue(const std::string& text)
{
	OutputWithConsoleAttributes(text, BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void ConUtils::OutputGreen(const std::string& text, bool highlight, bool newLineAtEnd)
{
	if (highlight)
		OutputWithConsoleAttributes(text, FOREGROUND_GREEN | FOREGROUND_INTENSITY, newLineAtEnd);
	else
		OutputWithConsoleAttributes(text, FOREGROUND_GREEN, newLineAtEnd);
}

void ConUtils::OutputYellow(const std::string& text)
{
	OutputWithConsoleAttributes(text, BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY);
}
