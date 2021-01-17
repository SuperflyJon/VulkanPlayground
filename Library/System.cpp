#include "stdafx.h"
#include "System.h"
#include "Common.h"
#include "Application.h"
#include "Extensions.h"
#include "WinUtil.h"

VulkanSystem::VulkanSystem()
	: physicalDevice(nullptr), deviceProps(nullptr), deviceFeatures{}, queueIndicies{ }, device(nullptr), debugOutputIndent(0), requestedDeviceFeatures{}, fontsEnabled(true)
{
}

VkFormat VulkanSystem::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (auto format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			return format;
	}
	return VK_FORMAT_UNDEFINED;
}

void VulkanSystem::FindDevice(VkInstance instance, VkSurfaceKHR windowSurface, const Extensions& extensions)
{
	physicalDevice = VulkanPlayground::FindPhysicalDevice(instance, windowSurface, queueIndicies, extensions);

	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	if (VulkanPlayground::showObjectCreationMessages)
	{
		std::cout << "Selected: " << VulkanPlayground::GetDeviceDetailsString(physicalDevice) << std::endl;
		if (queueIndicies.graphicsFamily == queueIndicies.presentFamily && queueIndicies.presentFamily == queueIndicies.transferFamily)
			std::cout << "Graphics/Present/Transfer queue: " << queueIndicies.graphicsFamily << "\n";
		else
			std::cout << "Graphics queue: " << queueIndicies.graphicsFamily << ", Present queue: " << queueIndicies.presentFamily << ", Transfer queue: " << queueIndicies.transferFamily << "\n";
	}

	if (VulkanPlayground::nameObjects && extensions.CheckIfInstanceExtensionEnabled(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
		debugMarker.Init(instance);
}

void VulkanSystem::Setup(const Extensions& extensions, VulkanApplication &app)
{
	VkPhysicalDeviceFeatures requiredDeviceFeatures{};
	app.GetRequiredDeviceFeatures(deviceFeatures, &requiredDeviceFeatures);
	if (fontsEnabled)
		requiredDeviceFeatures.geometryShader = VK_TRUE;	// Fonts use geometry shader
	if (memcmp(&requiredDeviceFeatures, &requestedDeviceFeatures, sizeof(requiredDeviceFeatures)) != 0)
		TidyUp();

	if (device == nullptr)
	{
		device = VulkanPlayground::CreateLogicalDevice(physicalDevice, queueIndicies, extensions, requiredDeviceFeatures);
		requestedDeviceFeatures = requiredDeviceFeatures;
		DebugNameObject(physicalDevice, VK_OBJECT_TYPE_PHYSICAL_DEVICE, "Physical Device", VulkanPlayground::GetDeviceDetailsName(physicalDevice));
		auto incOutput = GetScopedDebugOutputIncrement();
		DebugNameObject(device, VK_OBJECT_TYPE_DEVICE, "Logical Device", "");
		auto incOutput2 = GetScopedDebugOutputIncrement();
		bufMan.Setup(*this);
	}
}

uint32_t VulkanSystem::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i))
		{
			if ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
	}

	throw std::runtime_error("Failed to find suitable memory!");
}

void VulkanSystem::DeviceWaitIdle()
{
	CHECK_VULKAN(vkDeviceWaitIdle(device), "Failed to wait for device");
}

void VulkanSystem::TidyUp()
{
	bufMan.Tidy(*this);
	if (device != nullptr)
	{
		for (auto shaderModule : shaderModules)
			vkDestroyShaderModule(device, shaderModule.second, nullptr);
		shaderModules.clear();

		vkDestroyDevice(device, nullptr);
		device = nullptr;
	}
}

bool VulkanSystem::FindModule(const std::string& shaderFilename, VkShaderModule& shaderModule)
{
	auto pos = shaderModules.find(shaderFilename);
	if (pos != shaderModules.end())
	{
		shaderModule = pos->second;
		return true;
	}
	else
	{
		shaderModule = VulkanPlayground::LoadShaderModule(*this, shaderFilename);
		shaderModules[shaderFilename] = shaderModule;
		return false;
	}
}

void DebugMarker::Init(VkInstance instance)
{
	vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
	vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT");
	vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
	vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
	vkQueueInsertDebugUtilsLabelEXT = (PFN_vkQueueInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkQueueInsertDebugUtilsLabelEXT");
	vkQueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkQueueBeginDebugUtilsLabelEXT");
	vkQueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkQueueEndDebugUtilsLabelEXT");
}

const char* GetObjectName(VkObjectType objectType)
{
	switch (objectType)
	{
	case VK_OBJECT_TYPE_UNKNOWN:	return "Unknown / Undefined Handle";
	case VK_OBJECT_TYPE_INSTANCE:	return "VkInstance";
	case VK_OBJECT_TYPE_PHYSICAL_DEVICE:	return "VkPhysicalDevice";
	case VK_OBJECT_TYPE_DEVICE:	return "VkDevice";
	case VK_OBJECT_TYPE_QUEUE:	return "VkQueue";
	case VK_OBJECT_TYPE_SEMAPHORE:	return "VkSemaphore";
	case VK_OBJECT_TYPE_COMMAND_BUFFER:	return "VkCommandBuffer";
	case VK_OBJECT_TYPE_FENCE:	return "VkFence";
	case VK_OBJECT_TYPE_DEVICE_MEMORY:	return "VkDeviceMemory";
	case VK_OBJECT_TYPE_BUFFER:	return "VkBuffer";
	case VK_OBJECT_TYPE_IMAGE:	return "VkImage";
	case VK_OBJECT_TYPE_EVENT:	return "VkEvent";
	case VK_OBJECT_TYPE_QUERY_POOL:	return "VkQueryPool";
	case VK_OBJECT_TYPE_BUFFER_VIEW:	return "VkBufferView";
	case VK_OBJECT_TYPE_IMAGE_VIEW:	return "VkImageView";
	case VK_OBJECT_TYPE_SHADER_MODULE:	return "VkShaderModule";
	case VK_OBJECT_TYPE_PIPELINE_CACHE:	return "VkPipelineCache";
	case VK_OBJECT_TYPE_PIPELINE_LAYOUT:	return "VkPipelineLayout";
	case VK_OBJECT_TYPE_RENDER_PASS:	return "VkRenderPass";
	case VK_OBJECT_TYPE_PIPELINE:	return "VkPipeline";
	case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:	return "VkDescriptorSetLayout";
	case VK_OBJECT_TYPE_SAMPLER:	return "VkSampler";
	case VK_OBJECT_TYPE_DESCRIPTOR_POOL:	return "VkDescriptorPool";
	case VK_OBJECT_TYPE_DESCRIPTOR_SET:	return "VkDescriptorSet";
	case VK_OBJECT_TYPE_FRAMEBUFFER:	return "VkFramebuffer";
	case VK_OBJECT_TYPE_COMMAND_POOL:	return "VkCommandPool";
	case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION:	return "VkSamplerYcbcrConversion";
	case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE:	return "VkDescriptorUpdateTemplate";
	case VK_OBJECT_TYPE_SURFACE_KHR:	return "VkSurfaceKHR";
	case VK_OBJECT_TYPE_SWAPCHAIN_KHR:	return "VkSwapchainKHR";
	case VK_OBJECT_TYPE_DISPLAY_KHR:	return "VkDisplayKHR";
	case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:	return "VkDisplayModeKHR";
	case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:	return "VkDebugReportCallbackEXT";
	case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV:	return "VkIndirectCommandsLayoutNV";
	case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:	return "VkDebugUtilsMessengerEXT";
	case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:	return "VkValidationCacheEXT";
	case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:	return "VkAccelerationStructureKHR";
	case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL:	return "VkPerformanceConfigurationINTEL";
	default: return "Unknown Type";
	}
}

void DebugMarker::NameObject(VkDevice device, void* object, VkObjectType objectType, uint32_t debugOutputIndent, const std::string& type, const std::string& desc) const
{
	std::string name = type;
	if (!desc.empty())
		name += " [" + desc + "]";

	if (vkSetDebugUtilsObjectNameEXT)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		nameInfo.objectType = objectType;
		nameInfo.objectHandle = (uint64_t)object;
		std::string testName = "#" + name;
		nameInfo.pObjectName = testName.c_str();
		CHECK_VULKAN(vkSetDebugUtilsObjectNameEXT(device, &nameInfo), "Failed to name object");
	}

	if (VulkanPlayground::showObjectCreationMessages)
	{
		WinUtils::OutputGreen(std::string(debugOutputIndent * 2, ' ') + name, true, false);
		std::stringstream ss;
		ss << " Using " << GetObjectName(objectType) << " (" << object << ")";
		WinUtils::OutputGreen(ss.str(), false);
	}
}

VkDebugUtilsLabelEXT GetDebugLabel(const std::string& label, const float colour[3])
{
	VkDebugUtilsLabelEXT labelInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
	labelInfo.pLabelName = label.c_str();
	if (colour && (colour[0] || colour[1] || colour[2]))
	{
		memcpy(labelInfo.color, colour, sizeof(float[3]));
		labelInfo.color[3] = 1.0f;
	}
	return labelInfo;
}

void DebugMarker::InsertLabel(VkCommandBuffer commandBuffer, const std::string& label, const float colour[3])
{
	if (vkCmdInsertDebugUtilsLabelEXT)
	{
		const auto& labelInfo = GetDebugLabel(label, colour);
		vkCmdInsertDebugUtilsLabelEXT(commandBuffer, &labelInfo);
	}
}

void DebugMarker::StartRegion(VkCommandBuffer commandBuffer, const std::string& label, const float colour[3])
{
	if (vkCmdBeginDebugUtilsLabelEXT)
	{
		const auto& labelInfo = GetDebugLabel(label, colour);
		vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &labelInfo);
	}
}

void DebugMarker::EndRegion(VkCommandBuffer commandBuffer)
{
	if (vkCmdEndDebugUtilsLabelEXT)
		vkCmdEndDebugUtilsLabelEXT(commandBuffer);
}

void DebugMarker::InsertLabel(VkQueue queue, const std::string& label, const float colour[3])
{
	if (vkQueueInsertDebugUtilsLabelEXT)
	{
		const auto& labelInfo = GetDebugLabel(label, colour);
		vkQueueInsertDebugUtilsLabelEXT(queue, &labelInfo);
	}
}

void DebugMarker::StartRegion(VkQueue queue, const std::string& label, const float colour[3])
{
	if (vkQueueBeginDebugUtilsLabelEXT)
	{
		const auto& labelInfo = GetDebugLabel(label, colour);
		vkQueueBeginDebugUtilsLabelEXT(queue, &labelInfo);
	}
}

void DebugMarker::EndRegion(VkQueue queue)
{
	if (vkQueueEndDebugUtilsLabelEXT)
		vkQueueEndDebugUtilsLabelEXT(queue);
}

ScopedDebugOutputIncrement::ScopedDebugOutputIncrement(VulkanSystem& _system, uint32_t _depth) : system(_system), depth(_depth)
{
	system.IncDebugOutput(depth);
}
ScopedDebugOutputIncrement::~ScopedDebugOutputIncrement()
{
	system.DecDebugOutput(depth);
}
