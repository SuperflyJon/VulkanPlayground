#pragma once

#include "Common.h"
#include "Image.h"
#include "DisplayBuffers.h"

class RenderPass;

struct SwapChainSupportDetails
{
	SwapChainSupportDetails() : capabilities{} {}
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class SwapChain : public ITidy
{
public:
	SwapChain();
	void Create(VulkanSystem &system, VkSurfaceKHR windowSurface, uint32_t width, uint32_t height, bool vSync, const std::string& debugName);
	void Tidy(VulkanSystem& system) override;

	void CreateFrameBuffers(VulkanSystem &system, RenderPasses& renderPasses, VkFormat depthBufferFormat, const std::string& debugName);
	bool DrawFrame(VulkanSystem& system, RenderPasses& renderPasses);

	static SwapChainSupportDetails GetSwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool vSync);

	VkExtent2D &GetWorkingExtent() { return workingExtent; }
	VkFormat GetImageFormat() const { return swapChainImageFormat.format; }
	uint32_t GetImageCount() const { return imageCount; }

private:
	VkSwapchainKHR swapChain;
	SwapChainSupportDetails swapChainDetails;
	VkSurfaceFormatKHR swapChainImageFormat;
	VkExtent2D workingExtent;

	uint32_t imageCount;

	VkQueue graphicsQueue{};
	VkQueue presentQueue{};
};
