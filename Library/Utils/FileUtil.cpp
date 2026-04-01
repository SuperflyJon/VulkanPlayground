#include "pch.h"
#include "FileUtil.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <io.h>
#include <iostream>

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

std::string FileUtils::GetFileNode(std::string path)
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

bool FileUtils::FileExists(const std::string& filename)
{
	return (_access(filename.c_str(), 0) == 0);
}

std::string FileUtils::FindFile(const std::string& filename)
{
	std::vector<std::string> dirs{ "bin", "..\\bin", "..\\..\\bin", "..\\..\\..\\bin", ". ", "..", "..\\..", "..\\..\\..", "..\\..\\..\\.." };
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

std::string FileUtils::GetExtension(const std::string& filename)
{
	std::string::size_type dot = filename.find_last_of(".");
	if (dot == std::string::npos)
		return "";

	std::string ext = filename.substr(dot + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), [](char c) {return static_cast<char>(std::tolower(c)); });
	return ext;
}

std::string FileUtils::GetJustFileName(const std::string& filename)
{
	std::string name = GetFileNode(filename);

	std::string::size_type dot = name.find_last_of(".");
	if (dot == std::string::npos)
		return name;
	else
		return name.substr(0, dot);
}

void* FileUtils::GetProcAddress(void* mod, const std::string& funName)
{
	return ::GetProcAddress((HMODULE)mod, funName.c_str());
}

FileUtils::DelayedLib::DelayedLib(const std::string& _moduleName)
	: mod(nullptr), moduleName(_moduleName)
{
}

FileUtils::DelayedLib::~DelayedLib()
{
	if (mod)
		FreeLibrary((HMODULE)mod);
}

void FileUtils::DelayedLib::Setup()
{
	if (mod == nullptr)
	{
		mod = LoadLibraryA(FileUtils::FindFile(moduleName).c_str());
		if (mod == nullptr)
			throw std::runtime_error("Failed to find file: " + moduleName);

		Init();

		std::cout << "Loaded: " + moduleName + "\n";
	}
}
