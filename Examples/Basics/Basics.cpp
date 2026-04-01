#include <VulkanPlayground\Includes.h>
#include <VulkanPlayground\WindowSystem.h>
#include "Utils\ConUtil.h"
#include "Utils\StrUtil.h"
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
	ConUtils::EnableUnicodeConsoleOutput();

	try
	{
		AppGroups appGroups;
		WindowSystem windowSystem(1200, 800, "Basic Examples", CheckKeys, &appGroups);
		do
		{
			auto& app = appGroups.GetCurrentApp();
			app.GetTextHelper()->LoadStrings(windowSystem.GetVulkanSystem());
			ConUtils::OutputYellow(StrUtils::UTF32toUTF8(((BasicStrings*)app.GetTextHelper())->GetTitle()));

			windowSystem.RunApp(app);
		} while (windowSystem.Running() && appGroups.SceneChanged());

		windowSystem.Close();
	}
	catch (std::exception& err)
	{
		ConUtils::OutputError("Fatal error: " + std::string(err.what()));
		return false;
	}
}

BasicStrings::BasicStrings(int resVal)
	: stringBase(resVal)
{}
BasicStrings::~BasicStrings() = default;

void BasicStrings::LoadStrings(VulkanSystem& system)
{
	pSystem = &system;
	if (stringBase != 0 && appTitle.empty())
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

void BasicStrings::PrintMainText(bool app3D, bool lightApp)
{
	if (!appTitle.empty())
	{
		PrintString(*mainText, groupString, glm::vec3(0.75f), 0.6f);
		
		static Vulkan2DFont headingFont(*pSystem, "Times New Roman", 48);
		PrintStringFont(headingFont, *mainText, appTitle, glm::vec3(1, 1, .3));
		PrintString(*mainText, subGroupString, glm::vec3(0.75f), 0.6f);

		PrintString(*mainText, U"\n\n" + appDesc);
		PrintString(*mainText, U"\n\n" + basicKeyString, glm::vec3(0.75f), 0.6f);
	}

	PrintKeySection(appKeys);

	TextHelperBase::PrintMainText(app3D, lightApp);
}
