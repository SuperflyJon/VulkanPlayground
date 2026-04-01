#pragma once

#include <VulkanPlayground/TextHelper.h>

class AppGroups;

class BasicStrings : public TextHelperBase
{
public:
	BasicStrings(int resVal);
	~BasicStrings();

	void LoadStrings(VulkanSystem& system) override;
	void PrintMainText(bool app3D, bool lightApp) override;
	void SetGroupStrings(const std::string& groups, const std::string& subGroup)
	{
		groupString = groups;
		subGroupString = subGroup;
	}
	const UnicodeString& GetTitle() const { return appTitle; }

private:
	int stringBase;
	UnicodeString groupString, subGroupString;

	UnicodeString basicKeyString, appTitle, appDesc, appKeys;
};
