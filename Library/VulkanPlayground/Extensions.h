#pragma once

#include <vector>
#include "DebugCallback.h"

class Extensions
{
public:
	void Setup(const std::vector<const char *>& glfwExtensions);

	void SetupDebugCallback(VkInstance _instance, DebugCallback::CallbackFn callback) { debugCallback.Setup(_instance, callback); }
	void TidyUpDebugCallback() { debugCallback.Destroy(); }

	bool CheckValidationLayerSupport() const;
	bool CheckExtensionSupport() const;
	void AddInstanceExtensions(VkInstanceCreateInfo& instanceInfo) const;
	void AddDeviceExtensions(VkDeviceCreateInfo& deviceInfo) const;

	void AddValidationLayer(const char* layer) { validationLayers.push_back(layer); }
	void AddReqExtension(const char* extension) { reqExtensions.push_back(extension); }
	void AddReqDeviceExtension(const char* extension) { reqDeviceExtensions.push_back(extension); }

	bool CheckIfInstanceExtensionEnabled(const char* extension) const;
	bool CheckIfDeviceExtensionEnabled(const char* extension) const;
	bool CheckReqDeviceExtensions(const std::vector<VkExtensionProperties>& deviceExtensions) const;
	void CheckSupported() const
	{
		if (!CheckValidationLayerSupport())
			throw std::runtime_error("Validation layer check failed!");

		if (!CheckExtensionSupport())
			throw std::runtime_error("Extension check failed!");
	}

private:
	std::vector<const char*> validationLayers;
	std::vector<const char*> reqExtensions;
	std::vector<const char*> reqDeviceExtensions;

	DebugCallback debugCallback;
};
