#pragma once

#include <vector>
#include <string>
#include <stdexcept>

namespace WinUtils
{
	std::string GetFontFilename(const std::string& fontFace);
	std::vector<std::string> GetLinkedFonts(const std::string& fontFace);
	std::string GetWindowsFontPath();
	std::wstring GetWindowTitle();
	bool DebuggerAttached();
	void Pause();
	bool IsDllLoaded(const std::string& dllName);
}
