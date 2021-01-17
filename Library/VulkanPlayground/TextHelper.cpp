
#include "stdafx.h"
#include "TextHelper.h"
#include "BitmapFont.h"
#include "Application.h"
#include "WinUtil.h"
#include "RenderPass.h"

ITextHelper::ITextHelper()
 : showText(true), defaultFont(nullptr)
{
}
ITextHelper::~ITextHelper() = default;

void ITextHelper::Tidy(VulkanSystem& system)
{
	std::set<Vulkan2DFont*> fonts;
	for (auto typeSetter : registeredTypeSetters)
	{
		typeSetter->Tidy(system);
		fonts.insert((Vulkan2DFont*)&typeSetter->GetFont());
	}
	for (auto font : fonts)
		font->ClearTypeSetters(system);
	registeredTypeSetters.clear();
}

void ITextHelper::ResetPrints(bool clear)
{
	for (auto typeSetter : registeredTypeSetters)
		typeSetter->Reset(clear);
}

TypeSetter& TextHelperBase::PrintString(TypeSetter& typeSetter, const UnicodeString& unicodeString, glm::vec3 fontColour, float scale)
{
	if (!ShowingText() || !typeSetter.Visible())
		return typeSetter;

	registeredTypeSetters.insert(&typeSetter);
	typeSetter.GetFont().PrintString(typeSetter, unicodeString.GetUtf32String(), fontColour, scale);

	return typeSetter;
}

TextHelperBase::TextHelperBase()
	: 
	generalKeys("General|F11,Toggle Full screen|R,Reset scene|P,Show FPS|Shift+P,Toggle vSync|Escape,exit"),
	movementKeys("3d movement|Left button + mouse,Look around|Right button + mouse,Rotate world|Mouse wheel,Zoom|WASD,Move around"),
	lightKeys("Light|1,Toggle ambient light|2,Toggle diffuse light|3,Toggle specular light")
{
}

void TextHelperBase::SetupText(VkExtent2D workingExtent)
{
	AddPrintString("fps:(1234567890)vSync");
	AddPrintString(generalKeys);
	AddPrintString(movementKeys);
	AddPrintString(lightKeys);

	if (!mainText)
		mainText = std::make_unique<TypeSetter>(GetFont());
	if (!keyText)
	{
		keyText = std::make_unique<TypeSetter>(GetFont());
		keyText->SetHorizontalAlignment(HorizontalAlignment::Right);
	}

	const size_t padding = TypeSetter::TYPESET_PADDING;
	size_t usableWidth = workingExtent.width - padding * 4;

	const size_t MAIN_MIN_WIDTH = 400;
	const size_t MAIN_MAX_WIDTH = 800;
	const size_t KEY_MIN_WIDTH = 200;
	const size_t KEY_MAX_WIDTH = 400;

	size_t mainWidth = 0, keyWidth = 0;

	if (usableWidth > MAIN_MIN_WIDTH)
	{
		mainWidth = MAIN_MIN_WIDTH;
		usableWidth -= MAIN_MIN_WIDTH;
		if (usableWidth > KEY_MIN_WIDTH)
		{
			keyWidth = KEY_MIN_WIDTH;
			usableWidth -= KEY_MIN_WIDTH;
		}
		while (usableWidth > 0 && (mainWidth < MAIN_MAX_WIDTH || keyWidth < KEY_MAX_WIDTH))
		{
			auto spare = (keyWidth < KEY_MAX_WIDTH) ? usableWidth / 2 : usableWidth;
			auto mainExtra = std::min(MAIN_MAX_WIDTH - mainWidth, spare);
			mainWidth += mainExtra;
			usableWidth -= mainExtra;
			auto keyExtra = std::min(KEY_MAX_WIDTH - keyWidth, usableWidth);
			keyWidth += keyExtra;
			usableWidth -= keyExtra;
		}
	}

	size_t usableHeight = workingExtent.height;

	const size_t KEY_MIN_HEIGHT = 300;
	const size_t MAIN_MIN_HEIGHT = 300;

	if (mainWidth < MAIN_MIN_WIDTH || usableHeight < MAIN_MIN_HEIGHT)
		mainWidth = 0;
	if (keyWidth < KEY_MIN_WIDTH || usableHeight < KEY_MIN_HEIGHT)
		keyWidth = 0;

	mainText->Init(5, workingExtent.height - padding, mainWidth, usableHeight);
	keyText->Init(workingExtent.width - keyWidth - padding, workingExtent.height - padding, keyWidth, usableHeight);
}

std::vector<std::u32string> Tokenise(const std::u32string& source, char32_t delim)
{
	std::vector<std::u32string> ret;

	size_t startPos = 0;
	size_t breakPos;
	while ((breakPos = source.find(delim, startPos)) != std::string::npos)
	{
		ret.push_back(source.substr(startPos, breakPos - startPos));
		startPos = breakPos + 1;	// Move to after delimiter
	}
	if (startPos != source.size())
		ret.push_back(source.substr(startPos));

	return ret;
}

void TextHelperBase::PrintKeySection(const UnicodeString& keyString)
{
	if (!showText || keyString.empty())
		return;

	auto keys = Tokenise(keyString.GetUtf32String(), u'|');
	if (keys.size() > 1)
	{
		if (!keyText->FirstPrint())
			PrintString(*keyText, U"\n").UpdateBlankSpace(2);	// Small gap between groups

		auto title = U"\t" + keys[0] + U"\n";
		PrintString(*keyText, title);
		for (auto it = keys.begin() + 1; it != keys.end(); it++)
		{
			auto breakPos = it->find(',');
			auto key = it->substr(0, breakPos);
			auto keyDesc = it->substr(breakPos + 1);
			PrintString(*keyText, key, glm::vec3(.7f, .7f, 1), 0.8f);
			PrintString(*keyText, U" " + keyDesc + U"\n", glm::vec3(0.8f), 0.8f);
		}
	}
}

void TextHelperBase::AddPrintString(Vulkan2DFont& font, const UnicodeString& string)
{
	if (showText)
		font.PreProcessString(string.GetUtf32String());
}

void TextHelperBase::PrintText(VulkanApplication& app)
{
	PrintKeySection(generalKeys);
	if (dynamic_cast<VulkanApplication3D*>(&app))
		PrintKeySection(movementKeys);
	if (dynamic_cast<VulkanApplication3DLight*>(&app))
		PrintKeySection(lightKeys);

	PrintString(*mainText, app.GetTitle());
}

Vulkan2DFont& ITextHelper::GetFont()
{
	if (defaultFont == nullptr)
		defaultFont = std::make_unique<Vulkan2DFont>();
	return *defaultFont;
}

void ITextHelper::CreateCommandBuffers(VulkanSystem& system, RenderPasses& renderPasses)
{
	renderPasses.CreateCmdBuffers(1, system,
		[this, &system](VkCommandBuffer commandBuffer)
		{
			system.DebugStartRegion(commandBuffer, "2dFont Drawing", { 0.0f, 1.0f, 1.0f });
			for (auto typeSetter : registeredTypeSetters)
				typeSetter->Draw(system, commandBuffer);
			system.DebugEndRegion(commandBuffer);
		}, "FontCmds");
}

void ITextHelper::Process(VulkanSystem& system, std::set<Vulkan2DFont*> &changedFonts, bool* redraw, bool *recreate)
{
	*redraw = false;
	*recreate = false;

	for (auto typeSetter : registeredTypeSetters)
	{
		if (typeSetter->LayoutVertical(system))
			*redraw = true;

		auto& font = typeSetter->GetFont();
		if (!font.IsValid())
			changedFonts.insert((Vulkan2DFont*)&font);

		if (font.IsValid() && font.UnexpectedChars())
			*recreate = true;
	}
}
