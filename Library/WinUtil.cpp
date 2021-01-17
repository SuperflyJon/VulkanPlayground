
#include "stdafx.h"
#include "WinUtil.h"
#include <Windows.h>
#include <io.h>
#include <algorithm>
#include <string> 

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

void WinUtils::OutputError(const std::string& text)
{
	OutputWithConsoleAttributes(text, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_INTENSITY, true, std::cerr);
}

void WinUtils::OutputWarning(const std::string& text)
{
	OutputWithConsoleAttributes(text, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_RED | BACKGROUND_INTENSITY, true, std::cerr);
}

void WinUtils::OutputBlue(const std::string& text)
{
	OutputWithConsoleAttributes(text, BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void WinUtils::OutputGreen(const std::string& text, bool highlight, bool newLineAtEnd)
{
	if (highlight)
		OutputWithConsoleAttributes(text, FOREGROUND_GREEN | FOREGROUND_INTENSITY, newLineAtEnd);
	else
		OutputWithConsoleAttributes(text, FOREGROUND_GREEN, newLineAtEnd);
}

void WinUtils::OutputYellow(const std::string& text)
{
	OutputWithConsoleAttributes(text, BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY);
}

std::string GetFullExeName()
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	return buffer;
}

void RemoveFileNode(std::string& path)
{
	std::string::size_type pos = path.find_last_of("\\/");
	/*std::string leaf = path.substr(pos + 1);*/
	path = path.substr(0, pos);
}

std::string GetFileNode(std::string path)
{
	std::string::size_type pos = path.find_last_of("\\/");
	return path.substr(pos + 1);
}

std::string GetExeDir()
{
	std::string exeDir = GetFullExeName();
	RemoveFileNode(exeDir);
	return exeDir;
}

bool WinUtils::FileExists(const std::string& filename)
{
	return (_access(filename.c_str(), 0) == 0);
}

std::string WinUtils::FindFile(const std::string& filename)
{
	std::vector<std::string> dirs{ "bin", VULKAN_PLAYGROUND_DIR"\\bin", "..\\bin", "..\\..\\bin", "..\\..\\..\\bin", ". ", "..", "..\\..", "..\\..\\..", "..\\..\\..\\.." };
	for (auto dir : dirs)
	{
		std::string testfile{ dir + '\\' + filename };
		if (_access(testfile.c_str(), 0) == 0)
			return testfile;
	}
	std::string exeDir = GetExeDir();
	for (auto dir : dirs)
	{
		std::string testfile{ exeDir + '\\' + dir + '\\' + filename };
		if (_access(testfile.c_str(), 0) == 0)
			return testfile;
	}
	return filename;	// Might be in search path
}

std::string WinUtils::GetExtension(const std::string& filename)
{
	std::string::size_type dot = filename.find_last_of(".");
	if (dot == std::string::npos)
		return "";

	std::string ext = filename.substr(dot + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), [](char c) {return static_cast<char>(std::tolower(c)); });
	return ext;
}

std::string WinUtils::ThousandSep(size_t bigNumber)
{
	std::ostringstream ss;
	/*std::locale prev =*/ ss.imbue(std::locale(""));
	ss << bigNumber;
	return ss.str();
}

std::string WinUtils::FormatDataSize(size_t bytes)
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

bool CaselessCompare(const std::string& lhs, const std::string& rhs)
{
	return (_stricmp(lhs.c_str(), rhs.c_str()) == 0);
}

#include <psapi.h>
bool WinUtils::IsDllLoaded(const std::string& dllName)
{
	bool ret = false;

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
	if (hProcess != NULL)
	{
		HMODULE hMods[1024];
		DWORD cbNeeded;
		// Get a list of all the modules in this process.
		if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
		{
			for (size_t i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
			{
				char szModName[MAX_PATH];

				// Get the full path to the module's file.
				if (GetModuleFileNameExA(hProcess, hMods[i], szModName, sizeof(szModName)))
				{
					if (CaselessCompare(GetFileNode(szModName), dllName))
						ret = true;
				}
			}
		}
		CloseHandle(hProcess);
	}

	return ret;
}

std::string WinUtils::GetJustFileName(const std::string& filename)
{
	std::string name = GetFileNode(filename);

	std::string::size_type dot = name.find_last_of(".");
	if (dot == std::string::npos)
		return name;
	else
		return name.substr(0, dot);
}

#include "Registry.hpp"
#include "Common.h"

std::wstring FONT_REG_KEY = LR"(SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts)";
std::wstring LINKFONT_REG_KEY = LR"(SOFTWARE\Microsoft\Windows NT\CurrentVersion\FontLink\SystemLink)";

std::string WinUtils::GetFontFilename(const std::string& fontFace)
{
	auto fontFaceU16{ Widen(fontFace) };
	auto fontFaceTTU16{ fontFaceU16 + L" (TrueType)" };
	auto key = win32::RegOpenKey(HKEY_LOCAL_MACHINE, FONT_REG_KEY, KEY_READ);
	for (auto& val : win32::RegEnumValues(key.Get()))
	{
		if (val.first == fontFaceU16 || val.first == fontFaceTTU16)
		{
			if (val.second != REG_SZ)
				throw std::runtime_error("Error finding font name in registry");
			return Narrow(win32::RegGetString(key.Get(), L"", val.first));
		}
	}
	throw std::runtime_error("Font not installed: " + fontFace);
}

std::vector<std::string> WinUtils::GetLinkedFonts(const std::string& fontFace)
{
	std::vector<std::string> linkedFonts;
	auto fontFaceU16{ Widen(fontFace) };
	try
	{
		auto key = win32::RegOpenKey(HKEY_LOCAL_MACHINE, LINKFONT_REG_KEY, KEY_READ);
		auto linkedfontsU16 = win32::RegGetMultiString(key.Get(), L"", fontFaceU16);

		for (auto& linkedFont : linkedfontsU16)
		{
			auto pos = linkedFont.find(L',');
			if (pos != std::string::npos)
				linkedFonts.push_back(Narrow(linkedFont.substr(0, pos)));
		}
	}
	catch (win32::RegistryError)
	{	// Ignore registry errors - no linked fonts found
	}

	return linkedFonts;
}

std::string WinUtils::GetWindowsFontPath()
{
	char windir[MAX_PATH];
	GetWindowsDirectoryA(windir, MAX_PATH);
	return windir + std::string("\\fonts\\");
}

void* WinUtils::GetProcAddress(void* mod, const std::string& funName)
{
	return ::GetProcAddress((HMODULE)mod, funName.c_str());
}

WinUtils::DelayedLib::DelayedLib(const std::string& _moduleName)
	: mod(nullptr), moduleName(_moduleName) 
{
}

WinUtils::DelayedLib::~DelayedLib()
{
	if (mod)
		FreeLibrary((HMODULE)mod);
}

void WinUtils::DelayedLib::Setup()
{
	if (mod == nullptr)
	{
		mod = LoadLibraryA(WinUtils::FindFile(moduleName).c_str());
		if (mod == nullptr)
			throw std::runtime_error("Failed to find file: " + moduleName);

		Init();

		if (VulkanPlayground::showObjectCreationMessages)
			std::cout << "Loaded: " + moduleName + "\n";
	}
}

std::wstring WinUtils::LoadResourceString(int resourceID)
{
	HINSTANCE inst = GetModuleHandle(nullptr);
	wchar_t* pbuf = nullptr;
	size_t numChars = LoadStringW(inst, resourceID, (LPWSTR)&pbuf, 0);
	if (numChars == 0)
		throw std::runtime_error("Failed to load resource string!");
	return { pbuf, numChars };
}

std::u32string WinUtils::UTF16toUTF32(const std::wstring& utf16String)
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

std::wstring WinUtils::UTF32toUTF16(const std::u32string& utf32String)
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

std::string WinUtils::UTF32toUTF8(const std::u32string& utf32String)
{
	return Narrow(UTF32toUTF16(utf32String));
}

std::wstring WinUtils::Widen(const std::string& utf8String)
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

std::string WinUtils::Narrow(const std::wstring& utf16String)
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

bool WinUtils::DebuggerAttached()
{
	return IsDebuggerPresent() == TRUE;
}

void WinUtils::Pause()
{
	if (!DebuggerAttached())
		system("pause");
}

std::wstring WinUtils::GetWindowTitle()
{
	auto window = GetActiveWindow();
	auto size = GetWindowTextLengthW(window);
	std::wstring str;
	str.resize(size);
	GetWindowTextW(window, str.data(), size + 1);
	return str;
}
