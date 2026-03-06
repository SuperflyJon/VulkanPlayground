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
	WinUtils::EnableUnicodeConsoleOutput();

	try
	{
		AppGroups appGroups;
		WindowSystem windowSystem(1200, 800, "Basic Examples", CheckKeys, &appGroups);
		do
		{
			auto& app = appGroups.GetCurrentApp();
			app.GetTextHelper()->LoadStrings();
			WinUtils::OutputYellow(WinUtils::UTF32toUTF8(((BasicStrings*)app.GetTextHelper())->GetTitle()));

			windowSystem.RunApp(app);
		} while (windowSystem.Running() && appGroups.SceneChanged());

		windowSystem.Close();
	}
	catch (std::exception& err)
	{
		WinUtils::OutputError("Fatal error: " + std::string(err.what()));
		return false;
	}
}

BasicStrings::BasicStrings(int resVal)
	: stringBase(resVal)
{}
BasicStrings::~BasicStrings() = default;

void BasicStrings::LoadStrings()
{
	if (stringBase != 0)
	{
		appTitle.Load(stringBase);
		appDesc.Load(stringBase + 1);
		appKeys.Load(stringBase + 2);
		basicKeyString.Load(IDR_BasicKeys);

		GetFont().AddCharacters(U"(1234567890/)");	// For counters
		GetFont().AddCharacters(appKeys);
		generalKeys = UnicodeString(generalKeys + U"|Arrow keys,Change scene");
	}
}

void BasicStrings::PrintText(bool app3D, bool lightApp)
{
	if (!showText)
		return;

	if (!appTitle.empty())
	{
		PrintStringHelper(*mainText, groupString, glm::vec3(0.75f), 0.6f);
		PrintStringHelper(*mainText, appTitle, glm::vec3(1, 1, .3), 2.0f);
		PrintStringHelper(*mainText, subGroupString, glm::vec3(0.75f), 0.6f);

		PrintStringHelper(*mainText, U"\n\n" + appDesc);
		PrintStringHelper(*mainText, U"\n\n" + basicKeyString, glm::vec3(0.75f), 0.6f);
	}

	PrintKeySection(appKeys);

	TextHelperBase::PrintText(app3D, lightApp);
}
