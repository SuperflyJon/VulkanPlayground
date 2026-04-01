#pragma once

#include <string>
#include <stdexcept>

namespace FileUtils
{
	bool FileExists(const std::string& filename);
	std::string FindFile(const std::string& filename);
	std::string GetJustFileName(const std::string& filename);
	std::string GetExtension(const std::string& filename);
	std::string GetFileNode(std::string path);

	void* GetProcAddress(void* mod, const std::string& funName);

	class DelayedLib
	{
	public:
		DelayedLib(const std::string& _moduleName);
		virtual ~DelayedLib();

		template<class Fun>
		void LoadFunction(Fun& fun, const std::string& funName)
		{
			fun = reinterpret_cast<Fun>(GetProcAddress(mod, funName));
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
