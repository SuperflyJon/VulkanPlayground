#include "stdafx.h"
#include "Extensions.h"
#include "Common.h"
#include "WinUtil.h"

void Extensions::Setup(const std::vector<const char *>& glfwExtensions)
{
	reqExtensions.insert(reqExtensions.begin(), glfwExtensions.begin(), glfwExtensions.end());
	reqDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	int showVulkanValidationMessages = VulkanPlayground::showVulkanValidationMessages;
	if (VulkanPlayground::failVulkanCallsOnError)
		showVulkanValidationMessages |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;	// To fail on errors, must show errors

	if (showVulkanValidationMessages > 0)
	{
		debugCallback.Enable((VkDebugUtilsMessageSeverityFlagBitsEXT)showVulkanValidationMessages, VulkanPlayground::failVulkanCallsOnError);

		std::vector<const char*> layers;

		if (WinUtils::IsDllLoaded("Nvda.Graphics.Injection.dll") || WinUtils::IsDllLoaded("RenderDoc.dll"))
		{	// Nb. other extensions can interfere with debugger
			std::cout << "Running with GPU debugger - debug markers enabled\n";
			//layers = { "VK_LAYER_NV_nsight" };	// Not sure this is actually needed by nsight...
			VulkanPlayground::nameObjects = true;
		}
		else
		{
			layers = { "VK_LAYER_KHRONOS_validation" };
		}

		for (auto& layer : layers)
			AddValidationLayer(layer);

		// Enable debug reporting
		AddReqExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	if (debugCallback.ShowLoaderMessages())
		CheckSupported();	// This takes a while, so only enable if loader messages enabled
}

bool Extensions::CheckValidationLayerSupport() const
{
	uint32_t vkLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&vkLayerCount, nullptr);
	std::vector<VkLayerProperties> vkLayers(vkLayerCount);
	vkEnumerateInstanceLayerProperties(&vkLayerCount, vkLayers.data());

	if (VulkanPlayground::showAvaliableObjects)
	{
		std::cout << "Availabe layers:\n";
		for (auto& layer : vkLayers)
		{
			std::cout << "\t" << layer.layerName << "\n";
		}
	}
	// Check layers are available
	bool okay = true;
	for (auto layer : validationLayers)
	{
		if (std::find_if(vkLayers.begin(), vkLayers.end(), [&](auto& elem) {return (std::string(elem.layerName) == layer); }) == vkLayers.end())
		{
			okay = false;
			std::cerr << "Required layer: " << layer << " not available!\n";
		}
	}

	return okay;
}

bool Extensions::CheckExtensionSupport() const
{
	uint32_t vkExtensionCount = 0;
	CHECK_VULKAN(vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, nullptr), "Enumerating Extensions");
	std::vector<VkExtensionProperties> vkExtensions(vkExtensionCount);
	CHECK_VULKAN(vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, vkExtensions.data()), "Enumerating Extensions");

	if (VulkanPlayground::showAvaliableObjects)
	{
		std::cout << "Availabe extensions:\n";
		for (auto& extension : vkExtensions)
		{
			std::cout << "\t" << extension.extensionName << "\n";
		}
	}

	// Check extensions are supported
	bool okay = true;
	for (auto extension : reqExtensions)
	{
		if (std::find_if(vkExtensions.begin(), vkExtensions.end(), [extension](auto& elem) {return (elem.extensionName == std::string(extension)); }) == vkExtensions.end())
		{
			okay = false;
			std::cerr << "Required extension: " << extension << " not supported!\n";
		}
	}
	return okay;
}

void Extensions::AddInstanceExtensions(VkInstanceCreateInfo& instanceInfo) const
{
	if (!reqExtensions.empty())
	{
		instanceInfo.enabledExtensionCount = (uint32_t)reqExtensions.size();
		instanceInfo.ppEnabledExtensionNames = reqExtensions.data();
	}
	if (!validationLayers.empty())
	{
		instanceInfo.enabledLayerCount = (uint32_t)validationLayers.size();
		instanceInfo.ppEnabledLayerNames = validationLayers.data();
	}
}

void Extensions::AddDeviceExtensions(VkDeviceCreateInfo& deviceInfo) const
{
	if (!validationLayers.empty())
	{
		deviceInfo.enabledLayerCount = (uint32_t)validationLayers.size();
		deviceInfo.ppEnabledLayerNames = validationLayers.data();
	}

	if (!reqDeviceExtensions.empty())
	{
		deviceInfo.enabledExtensionCount = (uint32_t)reqDeviceExtensions.size();
		deviceInfo.ppEnabledExtensionNames = reqDeviceExtensions.data();
	}
}

bool Extensions::CheckReqDeviceExtensions(const std::vector<VkExtensionProperties>& deviceExtensions) const
{
	bool okay = true;
	for (auto extension : reqDeviceExtensions)
	{
		if (std::find_if(deviceExtensions.begin(), deviceExtensions.end(), [extension](auto& elem) {return (elem.extensionName == std::string(extension)); }) == deviceExtensions.end())
		{
			okay = false;
			std::cerr << "Required extension: " << extension << " not supported!\n";
		}
	}
	return okay;
}

bool Extensions::CheckIfInstanceExtensionEnabled(const char* extension) const
{
	return (std::find(reqExtensions.begin(), reqExtensions.end(), extension) != reqExtensions.end());
}

bool Extensions::CheckIfDeviceExtensionEnabled(const char* extension) const
{
	return (std::find(reqDeviceExtensions.begin(), reqDeviceExtensions.end(), extension) != reqDeviceExtensions.end());
}
