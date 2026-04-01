#pragma once

#include <string>

namespace StrUtils
{
	std::wstring LoadResourceString(int resourceID);
	std::wstring Widen(const std::string& utf8String);
	std::string Narrow(const std::wstring& utf16String);
	std::u32string UTF16toUTF32(const std::wstring& utf16String);
	std::wstring UTF32toUTF16(const std::u32string& utf32String);
	std::string UTF32toUTF8(const std::u32string& utf32String);
	std::string ThousandSep(size_t bigNumber);
	std::string FormatDataSize(size_t bytes);

	bool CaselessCompare(const std::wstring& lhs, const std::wstring& rhs);
	bool CaselessCompare(const std::string& lhs, const std::string& rhs);
}
