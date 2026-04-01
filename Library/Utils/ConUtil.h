#pragma once

#include <string>

namespace ConUtils
{
	void EnableUnicodeConsoleOutput();
	void OutputError(const std::string& text);
	void OutputWarning(const std::string& text);
	void OutputBlue(const std::string& text);
	void OutputGreen(const std::string& text, bool highlight, bool newLineAtEnd = true);
	void OutputYellow(const std::string& text);
}
