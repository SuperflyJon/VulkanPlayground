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
	virtual void PrintText(VulkanApplication& /*app*/) {}

	virtual TypeSetter& PrintString(TypeSetter& typeSetter, const UnicodeString& /*unicodeString*/, glm::vec3 /*fontColour*/ = glm::vec3(1.0f, 1.0f, 1.0f), float /*scale*/ = 1.0f) { return typeSetter; }

	bool ShowingText() const { return showText; }
	void ToggleShowingText() { showText = !showText; }

	virtual void AddPrintString(Vulkan2DFont& /*font*/, const UnicodeString& /*string*/) {}
	virtual void AddPrintString(const UnicodeString& /*string*/) {}

	void Process(VulkanSystem& system, std::set<Vulkan2DFont*>& changedFonts, bool* redraw, bool* recreate);
	void CreateCommandBuffers(VulkanSystem& system, RenderPasses& renderPasses);

	Vulkan2DFont& GetFont();

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
	void PrintText(VulkanApplication& app) override;
	TypeSetter& PrintString(TypeSetter& typeSetter, const UnicodeString& unicodeString, glm::vec3 fontColour = glm::vec3(1.0f, 1.0f, 1.0f), float scale = 1.0f) override;

	void AddPrintString(Vulkan2DFont& font, const UnicodeString& string) override;
	void AddPrintString(const UnicodeString& string) override { AddPrintString(GetFont(), string); }
	
protected:
	std::unique_ptr<TypeSetter> mainText, keyText;
	UnicodeString generalKeys, movementKeys, lightKeys;
};
