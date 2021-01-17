#pragma once

#include <VulkanPlayground/TextHelper.h>

class AppGroups;

class BasicStrings : public TextHelperBase
{
public:
	BasicStrings(AppGroups& appGroups, const std::string& _appGroup, const std::string& _appName, int resVal);
	~BasicStrings();

	void LoadStrings() override;
	void PrintText(VulkanApplication& app) override;

private:

	AppGroups& appGroups;
	std::string appGroup, appName;
	int stringBase;

	UnicodeString basicKeyString, appTitle, appDesc, appKeys;
};
