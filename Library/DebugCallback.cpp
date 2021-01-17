#include "stdafx.h"
#include "DebugCallback.h"
#include "Extensions.h"
#include "Common.h"

static VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func)
		return func(instance, pCreateInfo, pAllocator, pCallback);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func)
		func(instance, callback, pAllocator);
}

void DebugCallback::Enable(VkDebugUtilsMessageSeverityFlagBitsEXT debugReportLevel, bool failOnError)
{
	enabled = true;
	reportLevel = debugReportLevel;
	abortOperationOnError = failOnError;
}

void DebugCallback::Setup(VkInstance _instance, CallbackFn callback)
{
	if (enabled)
	{
		// Remember for later
		userCallback = callback;
		instance = _instance;

		VkDebugUtilsMessengerCreateInfoEXT debugReportCallbackInfo { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		debugReportCallbackInfo.messageSeverity = reportLevel;
		debugReportCallbackInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugReportCallbackInfo.pUserData = this;
		debugReportCallbackInfo.pfnUserCallback = DebugCallbackFn;

		CHECK_VULKAN(CreateDebugReportCallbackEXT(instance, &debugReportCallbackInfo, nullptr, &vulkanCallback), "Failed to setup debug callback!");
	}
}

void DebugCallback::Destroy()
{
	if (enabled)
	{
		DestroyDebugReportCallbackEXT(instance, vulkanCallback, nullptr);
		enabled = false;
	}
}

bool DebugCallback::Message(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const char* pMessage, int32_t /*messageIdNumber*/, const char* /*pMessageIdName*/)
{
	if (!userCallback)
		return false;

	if ((reportLevel & messageSeverity) != messageSeverity)
		return false;

	if (messageTypes == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT && messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT && !showLoaderMessages)
		return false;

	userCallback(messageSeverity, pMessage);

	return abortOperationOnError;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback::DebugCallbackFn(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	return static_cast<DebugCallback*>(pUserData)->Message(messageSeverity, messageTypes, pCallbackData->pMessage, pCallbackData->messageIdNumber, pCallbackData->pMessageIdName);
}
