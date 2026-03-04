#pragma once

#include <VulkanPlayground/TextHelper.h>

class AppGroups;

class BasicStrings : public TextHelperBase
{
public:
	BasicStrings(int resVal);
	~BasicStrings();

	void LoadStrings() override;
	void PrintText(bool app3D, bool lightApp) override;
	void SetGroupStrings(const std::string& groups, const std::string& subGroup)
	{
		groupString = groups;
		subGroupString = subGroup;
	}
	const UnicodeString& GetTitle() const { return appTitle; }

private:
	int stringBase;
	std::string groupString, subGroupString;

	UnicodeString basicKeyString, appTitle, appDesc, appKeys;
};
