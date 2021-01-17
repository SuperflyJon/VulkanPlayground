#pragma once

class Extensions;
class VulkanApplication;

#include "Buffers.h"

struct QueueIndicies
{
	uint32_t graphicsFamily;
	uint32_t presentFamily;
	uint32_t transferFamily;
};

class DebugMarker
{
public:
	DebugMarker() : vkSetDebugUtilsObjectNameEXT(nullptr), vkCmdInsertDebugUtilsLabelEXT(nullptr), vkCmdBeginDebugUtilsLabelEXT(nullptr), vkCmdEndDebugUtilsLabelEXT(nullptr), vkQueueInsertDebugUtilsLabelEXT(nullptr), vkQueueBeginDebugUtilsLabelEXT(nullptr), vkQueueEndDebugUtilsLabelEXT(nullptr)
	{}
	void Init(VkInstance instance);
	void NameObject(VkDevice device, void* object, VkObjectType objectType, uint32_t debugOutputIndent, const std::string& type, const std::string& desc) const;
	void InsertLabel(VkCommandBuffer commandBuffer, const std::string& label, const float colour[3]);
	void StartRegion(VkCommandBuffer commandBuffer, const std::string& label, const float colour[3]);
	void EndRegion(VkCommandBuffer commandBuffer);
	void InsertLabel(VkQueue queue, const std::string& label, const float colour[3]);
	void StartRegion(VkQueue queue, const std::string& label, const float colour[3]);
	void EndRegion(VkQueue queue);

private:
	PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
	PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT;
	PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
	PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT;
	PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueInsertDebugUtilsLabelEXT;
	PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT;
	PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT;
};

class ScopedDebugOutputIncrement
{
public:
	explicit ScopedDebugOutputIncrement(VulkanSystem& _system, uint32_t depth = 1);
	~ScopedDebugOutputIncrement();
private:
	uint32_t depth;
	VulkanSystem& system;
};

class VulkanSystem
{
public:
	VulkanSystem();

	void FindDevice(VkInstance instance, VkSurfaceKHR windowSurface, const Extensions& extensions);
	void Setup(const Extensions& extensions, VulkanApplication& app);
	void TidyUp();
	bool FindModule(const std::string& shaderFilename, VkShaderModule& shaderModule);

	VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
	QueueIndicies GetQueueIndicies() const { return queueIndicies; }
	BufferManager& GetBufMan() { return bufMan; }
	void CreateGpuImage(VulkanSystem& system, Image& image, const PixelData& pd, VkImageUsageFlagBits usage, VkFormat format, uint32_t numFaces, const std::string& debugName, VkImageViewType imageType = VK_IMAGE_VIEW_TYPE_2D)
		{ bufMan.CreateGpuImage(system, image, pd, usage, format, numFaces, imageType, debugName);	}
	template <typename T> void CreateGpuBuffer(VulkanSystem& system, Buffer& buffer, const std::vector<T>& data, VkBufferUsageFlagBits usage, const std::string& debugName)
		{ bufMan.CreateGpuBuffer(system, buffer, data.data(), sizeof(data[0]) * data.size(), usage, debugName); 	}
	QueuePool &GetGraphicsQueuePool() { return bufMan.GetGraphicsQueuePool(); }
	VkDevice GetDevice() const { return device; }
	void DeviceWaitIdle();

	const VkPhysicalDeviceProperties& GetDeviceProperties()
	{
		if (deviceProps == nullptr)
		{
			deviceProps = new (VkPhysicalDeviceProperties);
			vkGetPhysicalDeviceProperties(physicalDevice, deviceProps);
		}
		return *deviceProps;
	}
	VkPhysicalDeviceFeatures GetDeviceFeatures() const { return deviceFeatures; }

	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void IncDebugOutput(uint32_t depth) { debugOutputIndent += depth; }
	void DecDebugOutput(uint32_t depth) { debugOutputIndent -= depth; }
	ScopedDebugOutputIncrement GetScopedDebugOutputIncrement(int depth = 1) {return ScopedDebugOutputIncrement(*this, depth); }
	void DebugNameObject(void* object, VkObjectType objectType, const std::string& type, const std::string& desc) const { debugMarker.NameObject(device, object, objectType, debugOutputIndent, type, desc); }
	typedef float ColType[3];
	void DebugInsertLabel(VkCommandBuffer commandBuffer, const std::string& label, const ColType& colour = ColType{ 0 }) { debugMarker.InsertLabel(commandBuffer, label, colour); }
	void DebugStartRegion(VkCommandBuffer commandBuffer, const std::string& label, const ColType& colour = ColType{ 0 }) { debugMarker.StartRegion(commandBuffer, label, colour); }
	void DebugEndRegion(VkCommandBuffer commandBuffer) { debugMarker.EndRegion(commandBuffer); }
	void DebugInsertLabel(VkQueue queue, const std::string& label, const ColType& colour = ColType{ 0 }) { debugMarker.InsertLabel(queue, label, colour); }
	void DebugStartRegion(VkQueue queue, const std::string& label, const ColType& colour = ColType{ 0 }) { debugMarker.StartRegion(queue, label, colour); }
	void DebugEndRegion(VkQueue queue) { debugMarker.EndRegion(queue); }

	bool fontsEnabled;

private:
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties* deviceProps;
	VkPhysicalDeviceFeatures deviceFeatures;
	QueueIndicies queueIndicies;
	BufferManager bufMan;
	uint32_t debugOutputIndent;
	DebugMarker debugMarker;
	VkDevice device;
	VkPhysicalDeviceFeatures requestedDeviceFeatures;
	std::map<std::string, VkShaderModule> shaderModules;
};
