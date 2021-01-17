#pragma once

#include <string>

class Extensions;

class DebugCallback
{
public:
	typedef void(*CallbackFn)(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, const std::string& pMessage);

	DebugCallback()
		: abortOperationOnError(false), enabled(false), reportLevel(VkDebugUtilsMessageSeverityFlagBitsEXT(0)), showLoaderMessages(false), vulkanCallback(nullptr), userCallback(nullptr), instance(nullptr)
	{
	}

	bool Message(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const char* pMessage, int32_t messageIdNumber, const char* pMessageIdName);

	/*PFN_vkDebugUtilsMessengerCallbackEXT*/
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFn(
		VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	void Enable(VkDebugUtilsMessageSeverityFlagBitsEXT debugReportLevel, bool failOnError);
	void Setup(VkInstance instance, CallbackFn _callback);
	void Destroy();

	void EnableLoaderMessages() { showLoaderMessages = true; }	// Enable to see loads of Vulkan loading messages
	bool ShowLoaderMessages() const { return showLoaderMessages; }

	bool Enabled() const { return enabled; }

private:
	bool abortOperationOnError;
	bool enabled;
	VkDebugUtilsMessageSeverityFlagBitsEXT reportLevel;
	bool showLoaderMessages;

	VkDebugUtilsMessengerEXT vulkanCallback;
	CallbackFn userCallback;
	VkInstance instance;
};
