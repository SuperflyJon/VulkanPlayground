#include "pch.h"
#include "StrUtil.h"
#include <Windows.h>
#include <stdexcept>
#include <memory>
#include <sstream>

std::wstring StrUtils::LoadResourceString(int resourceID)
{
	HINSTANCE inst = GetModuleHandle(nullptr);
	wchar_t* pbuf = nullptr;
	size_t numChars = LoadStringW(inst, resourceID, (LPWSTR)&pbuf, 0);
	if (numChars == 0)
		throw std::runtime_error("Failed to load resource string!");
	return { pbuf, numChars };
}

std::u32string StrUtils::UTF16toUTF32(const std::wstring& utf16String)
{
	std::u32string string32;
	string32.reserve(utf16String.size());

	for (auto it = utf16String.begin(); it != utf16String.end(); it++)
	{
		unsigned long c = (unsigned long)*it;
		if (c >= 0xd800 && c < 0xdc00)	// Surrogate pair?
		{
			it++;	// Second part of pair
			if (it == utf16String.end())
				break;	// Illegal string
			int c2 = *it;
			c = ((c & 0x3ff) << 10) + (c2 & 0x3ff) + 0x10000;
		}
		string32.push_back(c);
	}
	return string32;
}

std::wstring StrUtils::UTF32toUTF16(const std::u32string& utf32String)
{
	std::wstring result;
	for (auto codepoint : utf32String)
	{
		if (codepoint <= 0x0000FFFF)
			result.push_back((wchar_t)codepoint); /* normal case */
		else
		{	/* target is a character in range 0xFFFF - 0x10FFFF. (Surrogate pair) */
			static const int halfShift = 10;
			static const char32_t halfBase = 0x0010000UL;
			static const char32_t halfMask = 0x3FFUL;
			static const char32_t UNI_SUR_HIGH_START = 0xD800;
			static const char32_t UNI_SUR_LOW_START = 0xDC00;

			codepoint -= halfBase;
			result.push_back((wchar_t)((codepoint >> halfShift) + UNI_SUR_HIGH_START));
			result.push_back((wchar_t)((codepoint & halfMask) + UNI_SUR_LOW_START));
		}
	}
	return result;
}

std::string StrUtils::UTF32toUTF8(const std::u32string& utf32String)
{
	return Narrow(UTF32toUTF16(utf32String));
}

std::wstring StrUtils::Widen(const std::string& utf8String)
{
	if (utf8String.empty())
		return std::wstring();

	int wideSize = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8String.c_str(), (int)utf8String.size(), 0, 0);
	if (wideSize == 0)
		throw std::runtime_error("Invalid string found: " + utf8String);

	std::unique_ptr<wchar_t[]> wideBuf(new wchar_t[wideSize]);
	if (::MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), (int)utf8String.size(), wideBuf.get(), wideSize) == 0)
		throw std::runtime_error("Unable to translate string: " + utf8String);

	return std::wstring(wideBuf.get(), wideSize);
}

std::string StrUtils::Narrow(const std::wstring& utf16String)
{
	if (utf16String.empty())
		return std::string();

	int wideSize = (int)utf16String.size();
	int mbSize = ::WideCharToMultiByte(CP_UTF8, 0, utf16String.data(), wideSize, 0, 0, 0, 0);
	if (mbSize == 0)
		throw std::runtime_error("Unicode conversion failed");

	std::unique_ptr<char[]> mbBuf(new char[mbSize]);
	::WideCharToMultiByte(CP_UTF8, 0, utf16String.data(), wideSize, mbBuf.get(), mbSize, 0, 0);

	return std::string(mbBuf.get(), mbSize);
}

std::string StrUtils::ThousandSep(size_t bigNumber)
{
	std::ostringstream ss;
	/*std::locale prev =*/ ss.imbue(std::locale(""));
	ss << bigNumber;
	return ss.str();
}

std::string StrUtils::FormatDataSize(size_t bytes)
{
	std::stringstream ss;
	ss.setf(std::ios::fixed);
	ss.precision(2);
	if (bytes < 1000)
		ss << bytes << " bytes";
	else if (bytes < 1000 * 1024)
		ss << bytes / 1024.0 << "kb";
	else if (bytes < 1000 * 1024 * 1024)
		ss << bytes / (1024.0 * 1024.0) << "mb";
	return ss.str();
}

bool StrUtils::CaselessCompare(const std::wstring& lhs, const std::wstring& rhs)
{
	return (_wcsicmp(lhs.c_str(), rhs.c_str()) == 0);
}

bool StrUtils::CaselessCompare(const std::string& lhs, const std::string& rhs)
{
	return (_stricmp(lhs.c_str(), rhs.c_str()) == 0);
}
