#include "pch.h"
#include "WinUtil.h"
#include "StrUtil.h"
#include "FileUtil.h"
#include "Registry.hpp"
#include <psapi.h>

std::wstring FONT_REG_KEY = LR"(SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts)";
std::wstring LINKFONT_REG_KEY = LR"(SOFTWARE\Microsoft\Windows NT\CurrentVersion\FontLink\SystemLink)";

std::string WinUtils::GetFontFilename(const std::string& fontFace)
{
	auto fontFaceU16{ StrUtils::Widen(fontFace) };
	auto fontFaceTTU16{ fontFaceU16 + L" (TrueType)" };
	auto key = win32::RegOpenKey(HKEY_LOCAL_MACHINE, FONT_REG_KEY, KEY_READ);
	for (auto& val : win32::RegEnumValues(key.Get()))
	{
		if (StrUtils::CaselessCompare(val.first, fontFaceU16) || StrUtils::CaselessCompare(val.first, fontFaceTTU16))
		{
			if (val.second != REG_SZ)
				throw std::runtime_error("Error finding font name in registry");
			return StrUtils::Narrow(win32::RegGetString(key.Get(), L"", val.first));
		}
	}
	throw std::runtime_error("Font not installed: " + fontFace);
}

std::vector<std::string> WinUtils::GetLinkedFonts(const std::string& fontFace)
{
	std::vector<std::string> linkedFonts;
	auto fontFaceU16{ StrUtils::Widen(fontFace) };
	try
	{
		auto key = win32::RegOpenKey(HKEY_LOCAL_MACHINE, LINKFONT_REG_KEY, KEY_READ);
		auto linkedfontsU16 = win32::RegGetMultiString(key.Get(), L"", fontFaceU16);

		for (auto& linkedFont : linkedfontsU16)
		{
			auto pos = linkedFont.find(L',');
			if (pos != std::string::npos)
				linkedFonts.push_back(StrUtils::Narrow(linkedFont.substr(0, pos)));
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
					if (StrUtils::CaselessCompare(FileUtils::GetFileNode(szModName), dllName))
						ret = true;
				}
			}
		}
		CloseHandle(hProcess);
	}

	return ret;
}
