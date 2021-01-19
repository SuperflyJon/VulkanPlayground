#include <VulkanPlayground\Includes.h>
#include <VulkanPlayground\WindowSystem.h>
#include <VulkanPlayground\WinUtil.h>
#include "VulkanPlayground\BitmapFont.h"
#include "Basics.h"
#include "AppGroups.h"
#include "resource.h"

bool CheckKeys(GLFW& window, void* data)
{
	AppGroups& appGroups = *((AppGroups*)data);
	return appGroups.CheckKeys(window);
}

int main()
{
	WindowSystem windowSystem;
	AppGroups appGroups(windowSystem.GetVulkanSystem().fontsEnabled);
	windowSystem.InitWindow(1200, 800, "Basic Examples", CheckKeys, &appGroups);

	do
	{
		windowSystem.RunApp(appGroups.GetCurrentApp());
	} while (windowSystem.Running() || appGroups.SceneChanged());

	windowSystem.Close();
}

BasicStrings::BasicStrings(AppGroups& _appGroups, const std::string& _appGroup, const std::string& _appName, int resVal)
	: appGroups(_appGroups), appGroup(_appGroup), appName(_appName), stringBase(resVal)
{}
BasicStrings::~BasicStrings() = default;

void BasicStrings::LoadStrings()
{
	WinUtils::OutputYellow(appGroup + " - " + appName);
	
	if (stringBase != 0)
	{
		appTitle.Load(stringBase);
		appDesc.Load(stringBase + 1);
		appKeys.Load(stringBase + 2);
		basicKeyString.Load(IDR_BasicKeys);

		AddPrintString("(1234567890/)");	// For counters
		AddPrintString(appTitle);
		AddPrintString(appDesc);
		AddPrintString(appKeys);
		AddPrintString(basicKeyString);
		generalKeys = UnicodeString(generalKeys.GetUtf32String() + U"|Arrow keys,Change scene");
	}
}

void BasicStrings::PrintText(VulkanApplication& app)
{
	if (!showText)
		return;

	if (!appTitle.empty())
	{
		std::stringstream appNum;
		appNum << '(' << appGroups.GetCurrentGroupNum() << '/' << appGroups.GetNumGroups() << ") ";
		PrintString(*mainText, appNum.str(), glm::vec3(0.75f), 0.6f);
		PrintString(*mainText, appTitle, glm::vec3(1, 1, .3), 2.0f);
		std::stringstream appSubNum;
		appSubNum << " (" << appGroups.GetCurrentSubgroupNum() << '/' << appGroups.GetNumSubGroups() << ")";
		PrintString(*mainText, appSubNum.str(), glm::vec3(0.75f), 0.6f);

		PrintString(*mainText, U"\n\n" + appDesc.GetUtf32String());
		PrintString(*mainText, U"\n\n" + basicKeyString.GetUtf32String(), glm::vec3(0.75f), 0.6f);
	}

	if (appKeys.GetUtf32String().size() > 1)
		PrintKeySection(appKeys);

	TextHelperBase::PrintText(app);
}
