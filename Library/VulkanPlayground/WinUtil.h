#pragma once

namespace WinUtils
{
	void OutputError(const std::string& text);
	void OutputWarning(const std::string& text);
	void OutputBlue(const std::string& text);
	void OutputGreen(const std::string& text, bool highlight, bool newLineAtEnd = true);
	void OutputYellow(const std::string& text);
	bool FileExists(const std::string& filename);
	std::string FindFile(const std::string& filename);
	std::string GetExtension(const std::string& filename);
	std::string ThousandSep(size_t bigNumber);
	std::string FormatDataSize(size_t bytes);
	bool IsDllLoaded(const std::string& dllName);
	std::string GetJustFileName(const std::string& filename);
	std::string GetFontFilename(const std::string& fontFace);
	std::vector<std::string> GetLinkedFonts(const std::string& fontFace);
	std::string GetWindowsFontPath();
	std::wstring GetWindowTitle();
	void* GetProcAddress(void* mod, const std::string& funName);
	bool DebuggerAttached();
	void Pause();

	std::wstring LoadResourceString(int resourceID);
	std::wstring Widen(const std::string& utf8String);
	std::string Narrow(const std::wstring& utf16String);
	std::u32string UTF16toUTF32(const std::wstring& utf16String);
	std::wstring UTF32toUTF16(const std::u32string& utf32String);
	std::string UTF32toUTF8(const std::u32string& utf32String);

	class DelayedLib
	{
	public:
		DelayedLib(const std::string& _moduleName);
		virtual ~DelayedLib();

		template<class Fun>
		void LoadFunction(Fun& fun, const std::string& funName)
		{
			fun = reinterpret_cast<Fun>(WinUtils::GetProcAddress(mod, funName));
			if (fun == nullptr)
				throw std::runtime_error("Failed to delay load: " + moduleName + " function not found: " + funName);
		}

		void Setup();

	protected:
		virtual void Init() = 0;

		const std::string moduleName;
		void* mod;
	};
}
