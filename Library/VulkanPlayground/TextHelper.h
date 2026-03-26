#pragma once

#include "Common.h"

class TypeSetter;
class RenderPasses;
class FontData;

class ITextHelper : public ITidy
{
public:
	ITextHelper();
	virtual ~ITextHelper();
	void Tidy(VulkanSystem& system) override;
	void ResetPrints();

	virtual void LoadStrings() {}
	virtual void SetupText(VkExtent2D /*workingExtent*/) {}
	virtual void PrintKeySection(const UnicodeString& /*keyString*/) {}
	virtual void PrintText(bool /*App3D*/, bool /*lightApp*/) {}

	virtual TypeSetter& PrintStringFont(FontData& font, TypeSetter& typeSetter, const UnicodeString& unicodeString, glm::vec3 fontColour = glm::vec3(1.0f, 1.0f, 1.0f), float scale = 1.0f) = 0;
	TypeSetter& PrintString(TypeSetter& typeSetter, const UnicodeString& unicodeString, glm::vec3 fontColour = glm::vec3(1.0f, 1.0f, 1.0f), float scale = 1.0f)
	{
		return PrintStringFont(GetFont(), typeSetter, unicodeString, fontColour, scale);
	}

	bool ShowingText() const { return showText; }
	void ToggleShowingText() { showText = !showText; }

	void Process(VulkanSystem& system, bool* redraw, bool* recreate);
	void CreateFontCommandBuffers(VulkanSystem& system, RenderPass* fontRenderPass);

	FontData& GetFont();

	virtual bool Enabled() { return false; }

	const std::set<FontData*> &GetFontsInuse() const { return fontsInuse; }

protected:
	bool showText;
	std::unique_ptr<FontData> defaultFont;
	std::set<TypeSetter*> registeredTypeSetters;
	std::set<FontData*> fontsInuse;
};

class TextHelperBase : public ITextHelper
{
public:
	TextHelperBase();
	void SetupText(VkExtent2D workingExtent) override;
	void PrintKeySection(const UnicodeString& keyString) override;
	void PrintText(bool App3D, bool lightApp) override;
	TypeSetter& PrintStringFont(FontData& font, TypeSetter& typeSetter, const UnicodeString& unicodeString, glm::vec3 fontColour = glm::vec3(1.0f, 1.0f, 1.0f), float scale = 1.0f) override;
	

	bool Enabled() override { return true; }

protected:
	std::unique_ptr<TypeSetter> mainText, keyText;
	UnicodeString generalKeys, movementKeys, lightKeys;
};
