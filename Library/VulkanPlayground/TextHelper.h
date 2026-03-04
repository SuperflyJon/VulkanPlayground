#pragma once

#include "Common.h"

class TypeSetter;
class RenderPasses;

class ITextHelper : public ITidy
{
public:
	ITextHelper();
	virtual ~ITextHelper();
	void Tidy(VulkanSystem& system) override;
	void ResetPrints(bool clear);

	virtual void LoadStrings() {}
	virtual void SetupText(VkExtent2D /*workingExtent*/) {}
	virtual void PrintKeySection(const UnicodeString& /*keyString*/) {}
	virtual void PrintText(bool /*App3D*/, bool /*lightApp*/) {}

	virtual TypeSetter& PrintString(TypeSetter& typeSetter, const UnicodeString& /*unicodeString*/, glm::vec3 /*fontColour*/ = glm::vec3(1.0f, 1.0f, 1.0f), float /*scale*/ = 1.0f) { return typeSetter; }

	bool ShowingText() const { return showText; }
	void ToggleShowingText() { showText = !showText; }

	virtual void AddPrintString(Vulkan2DFont& /*font*/, const UnicodeString& /*string*/) {}
	virtual void AddPrintString(const UnicodeString& /*string*/) {}

	void Process(VulkanSystem& system, std::set<Vulkan2DFont*>& changedFonts, bool* redraw, bool* recreate);
	void CreateFontCommandBuffers(VulkanSystem& system, RenderPass* fontRenderPass);

	Vulkan2DFont& GetFont();

	virtual bool Enabled() { return false; }

protected:
	bool showText;
	std::unique_ptr<Vulkan2DFont> defaultFont;
	std::set<TypeSetter*> registeredTypeSetters;
};

class TextHelperBase : public ITextHelper
{
public:
	TextHelperBase();
	void SetupText(VkExtent2D workingExtent) override;
	void PrintKeySection(const UnicodeString& keyString) override;
	void PrintText(bool App3D, bool lightApp) override;
	TypeSetter& PrintString(TypeSetter& typeSetter, const UnicodeString& unicodeString, glm::vec3 fontColour = glm::vec3(1.0f, 1.0f, 1.0f), float scale = 1.0f) override;

	void AddPrintString(Vulkan2DFont& font, const UnicodeString& string) override;
	void AddPrintString(const UnicodeString& string) override { AddPrintString(GetFont(), string); }

	bool Enabled() override { return true; }

protected:
	std::unique_ptr<TypeSetter> mainText, keyText;
	UnicodeString generalKeys, movementKeys, lightKeys;
};
