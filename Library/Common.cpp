#include "stdafx.h"
#include "Common.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "Image.h"
#include "GLFW.h"
#include "Extensions.h"
#include "Pipeline.h"
#include "Application.h"
#include "WinUtil.h"
#include "System.h"
#include <regex>
#include <numeric>

namespace VulkanPlayground
{
	int showVulkanValidationMessages = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;	// Enable error and warning messages
	bool showAvaliableObjects = false;	// Enable to see avaliable Vulkan resources

#if _DEBUG
	bool showObjectCreationMessages = true;	// Enable to see main object lifetime messages
	bool failVulkanCallsOnError = true;	// Forces Vulkan functions that error to fail
#else
	bool showObjectCreationMessages = false;
	bool failVulkanCallsOnError = false;	// Just carry on and hope for the best
#endif
	bool nameObjects = false;

	int bufferCount = 2;	// 2 - double buffering, 3 - triple buffering

	const VkFormat offscreenColourBufferFormat = VK_FORMAT_R8G8B8A8_UNORM;

	const uint64_t NO_TIMEOUT = std::numeric_limits<uint64_t>::max();

	VkDeviceSize offsets[] = { 0 };
	const VkDeviceSize *zeroOffset = offsets;

	bool CheckVulkan(VkResult result, const char* errString, const char* file, int line, bool throwOnError)
	{
		if (result != VK_SUCCESS)
		{
			std::stringstream ss;
			ss << "VULKAN ERROR (" << result << "): " << errString << " in " << file << ":" << line << "\n";
			if (throwOnError)
				throw std::runtime_error(ss.str());
			else
				WinUtils::OutputError(ss.str());
		}
		return (result == VK_SUCCESS);
	}

	bool FindQueueFamilyIndexes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, QueueIndicies& indicies)
	{
		indicies.graphicsFamily = indicies.presentFamily = indicies.transferFamily = INVALID_VALUE;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		if (showAvaliableObjects)
			std::cout << "Found " << queueFamilyCount << " queues:\n";

		struct QueueTypes
		{
			VkBool32 graphics;
			VkBool32 present;
			VkBool32 transfer;
		};
		std::vector<QueueTypes> queueSupport(queueFamilies.size());
		for (uint32_t index = 0; index < queueFamilies.size(); index++)
		{
			auto& family = queueFamilies[index];
			auto& support = queueSupport[index];

			if (family.queueCount > 0)	// Probably always true
			{
				if (surface != nullptr)
					vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &support.present);
				else
					support.present = VK_TRUE;

				support.graphics = (family.queueFlags & VK_QUEUE_GRAPHICS_BIT);
				support.transfer = (family.queueFlags & VK_QUEUE_TRANSFER_BIT);
			}
			
			if (showAvaliableObjects)
			{
				std::cout << "\tQueue family " << index << " with " << family.queueCount << " queues holding:";
				if (support.present)
					std::cout << " [Present]";
				if (support.graphics)
					std::cout << " Graphics";
				if (support.transfer)
					std::cout << " Transfer";
				if (family.queueFlags & VK_QUEUE_COMPUTE_BIT)
					std::cout << " Compute";
				if (family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
					std::cout << " Sparse";
				std::cout << "\n";
			}
		}

#ifdef _DEBUG
		// Try using as many different queues as possible for debugging!
		int minSupported;
		int minQueue = INVALID_VALUE;
		do
		{
			minSupported = 0;
			for (uint32_t index = 0; index < queueFamilies.size(); index++)
			{
				int numSupported = 0;
				auto& support = queueSupport[index];
				if (indicies.graphicsFamily == INVALID_VALUE && support.graphics)
					numSupported++;
				if (indicies.presentFamily == INVALID_VALUE && support.present)
					numSupported++;
				if (indicies.transferFamily == INVALID_VALUE && support.transfer)
					numSupported++;

				if (numSupported > 0 && (minSupported == 0 || numSupported < minSupported))
				{
					minSupported = numSupported;
					minQueue = index;
				}
			}
			if (minSupported > 0)
			{
				auto& support = queueSupport[minQueue];
				if (indicies.graphicsFamily == INVALID_VALUE && support.graphics)
					indicies.graphicsFamily = minQueue;
				if (indicies.presentFamily == INVALID_VALUE && support.present)
					indicies.presentFamily = minQueue;
				if (indicies.transferFamily == INVALID_VALUE && support.transfer)
					indicies.transferFamily = minQueue;
			}
		} while (minSupported > 0);
#else
		// Try to find single queue with everything
		for (uint32_t index = 0; index < queueFamilies.size(); index++)
		{
			auto& support = queueSupport[index];
			if (support.graphics && support.present && support.transfer)
			{
				indicies.graphicsFamily = indicies.presentFamily = indicies.transferFamily = index;
				return true;
			}
		}

		// Just find things as they come
		for (uint32_t index = 0; index < queueFamilies.size(); index++)
		{
			auto& support = queueSupport[index];
			if (indicies.graphicsFamily == INVALID_VALUE && support.graphics)
				indicies.graphicsFamily = index;
			if (indicies.presentFamily == INVALID_VALUE && support.present)
				indicies.presentFamily = index;
			if (indicies.transferFamily == INVALID_VALUE && support.transfer)
				indicies.transferFamily = index;
		}
#endif
		return (indicies.graphicsFamily != INVALID_VALUE) && (indicies.presentFamily != INVALID_VALUE) && (indicies.transferFamily != INVALID_VALUE);
	}

	std::vector<char> ReadFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
			throw std::runtime_error("Failed to open file: " + filename);

		uint32_t fileSize = (uint32_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	std::string GetVendorName(uint32_t vendorID)
	{
		switch (vendorID)
		{
		case 0x1002:
			return "AMD";
		case 0x1010:
			return "ImgTec";
		case 0x10DE:
			return "NVidia";
		case 0x13B5:
			return "ARM";
		case 0x5143:
			return "Qualcomm";
		case 0x8086:
			return "Intel";
		default:
			return "Unknown";
		}
	}
	std::string GetDeviceTypeName(VkPhysicalDeviceType type)
	{
		switch (type)
		{
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			return "Other";
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			return "Integrated GPU";
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			return "Discrete GPU";
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			return "Virtual GPU";
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			return "CPU";
		default:
			return "Unknown";
		}
	}

	bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice, const Extensions& extensions)
	{
		uint32_t vkExtensionCount = 0;
		CHECK_VULKAN(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &vkExtensionCount, nullptr), "Enumerating Device Extensions");
		std::vector<VkExtensionProperties> vkExtensions(vkExtensionCount);
		CHECK_VULKAN(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &vkExtensionCount, vkExtensions.data()), "Enumerating Device Extensions");

		if (showAvaliableObjects)
		{
			std::cout << "Availabe device extensions:\n";
			for (auto& extension : vkExtensions)
			{
				std::cout << "\t" << extension.extensionName << "\n";
			}
		}
		// Check extensions are supported
		return extensions.CheckReqDeviceExtensions(vkExtensions);
	}

	std::string GetDeviceDetailsString(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties deviceProps;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);
		std::stringstream ss;
		ss << GetVendorName(deviceProps.vendorID) << " " << GetDeviceTypeName(deviceProps.deviceType) << ", " << deviceProps.deviceName << ", Vulkan " << VK_VERSION_MAJOR(deviceProps.apiVersion) << "." << VK_VERSION_MINOR(deviceProps.apiVersion) << " (DriverVersion: " << deviceProps.driverVersion << ")";
		return ss.str();
	}

	std::string GetDeviceDetailsName(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties deviceProps;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);
		return deviceProps.deviceName;
	}

	void DebugCallbackFn(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, const std::string& pMessage)
	{
		std::smatch m;
		std::regex_search(pMessage, m, std::regex(R"((.*\| MessageID = .+) \| (.*))"));
		std::string message;
		if (m.size() == 3)
		{
			WinUtils::OutputBlue(m[1]);
			message = m[2];
		}
		else
			message = pMessage;

		if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			std::regex_search(message, m, std::regex(R"((.*)(The Vulkan spec states.*))"));
			if (m.size() == 3)
			{
				WinUtils::OutputError(m[1]);
				WinUtils::OutputBlue(m[2]);
			}
			else
				WinUtils::OutputError(message);
		}
		else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			WinUtils::OutputWarning(message);
		else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
			std::cout << message;
		else
			WinUtils::OutputBlue(message);

		std::cout << std::endl;
	}

	VkInstance CreateInstance(const std::string& windowName, const Extensions& extensions)
	{
		// Create Instance
		VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
		appInfo.apiVersion = VK_API_VERSION_1_1;
		appInfo.pApplicationName = windowName.c_str();

		VkInstanceCreateInfo instanceInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instanceInfo.pApplicationInfo = &appInfo;
		extensions.AddInstanceExtensions(instanceInfo);

		VkInstance instance = nullptr;
		CHECK_VULKAN_THROW(vkCreateInstance(&instanceInfo, nullptr, &instance), "Failed to create instance");

		if (VulkanPlayground::showObjectCreationMessages)
			std::cout << "Vulkan Instance created ("<< instance << ")\n";
		return instance;
	}

	bool SwapChainSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		SwapChainSupportDetails swapChainDetails = SwapChain::GetSwapChainSupportDetails(physicalDevice, surface);
		return !swapChainDetails.formats.empty() && !swapChainDetails.presentModes.empty();
	}

	int RateDeviceSuitability(const VkPhysicalDeviceProperties& deviceProps, const VkPhysicalDeviceFeatures& deviceFeatures)
	{
		if (!deviceFeatures.geometryShader)
			return 0;

		int score = 0;
		if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 100000;

		if (deviceFeatures.samplerAnisotropy)
			score += 10000;

		score += deviceProps.limits.maxImageDimension2D;

		return score;
	}

	VkPhysicalDevice FindPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, QueueIndicies& queueIndicies, const Extensions& extensions)
	{
		// Pick a pyhsical device
		VkPhysicalDevice selectedPhysicalDevice = VK_NULL_HANDLE;

		uint32_t deviceCount = 0;
		CHECK_VULKAN(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr), "vkEnumeratePhysicalDevices");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		CHECK_VULKAN(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()), "vkEnumeratePhysicalDevices");

		if (showAvaliableObjects)
		{
			std::cout << "Found " << deviceCount << " Vulkan device" << ((deviceCount != 1) ? "s" : "") << ":\n";
			for (auto physicalDevice : devices)
			{
				std::cout << "\t" << GetDeviceDetailsString(physicalDevice) << std::endl;

				VkPhysicalDeviceFeatures deviceFeatures;
				vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
				if (deviceFeatures.geometryShader)
					std::cout << "\t\tGeometryShader" << std::endl;
			}
		}

		int bestScore = 0;
		for (auto physicalDevice : devices)
		{
			VkPhysicalDeviceProperties deviceProps;
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

			int score = RateDeviceSuitability(deviceProps, deviceFeatures);
			if (score > bestScore && CheckDeviceExtensionSupport(physicalDevice, extensions) && (surface == nullptr || SwapChainSuitable(physicalDevice, surface)))
			{
				QueueIndicies indicies;
				if (FindQueueFamilyIndexes(physicalDevice, surface, indicies))
				{
					queueIndicies = indicies;
					bestScore = score;
					selectedPhysicalDevice = physicalDevice;
				}
			}
		}
		if (selectedPhysicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("Failed to find a suitable device!");

		return selectedPhysicalDevice;
	}

	VkDevice CreateLogicalDevice(VkPhysicalDevice physicalDevice, QueueIndicies queueIndicies, const Extensions& extensions, const VkPhysicalDeviceFeatures& deviceFeatures)
	{
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamiles = { queueIndicies.graphicsFamily, queueIndicies.presentFamily, queueIndicies.transferFamily };
		float queuePriority = 1.0f;
		for (auto queueFamily : uniqueQueueFamiles)
		{
			VkDeviceQueueCreateInfo queueCreateInfo { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkDeviceCreateInfo deviceInfo { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
		deviceInfo.pEnabledFeatures = &deviceFeatures;

		extensions.AddDeviceExtensions(deviceInfo);

		VkDevice device;
		CHECK_VULKAN(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device), "Failed to create logical device!");
		return device;
	}

	VkShaderModule LoadShaderModule(const VulkanSystem& system, const std::string& filename)
	{
		std::cout << "Loading shader file: " << filename << "\n";
		auto shaderCode = ReadFile(filename);

		VkShaderModuleCreateInfo shaderModuleInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		shaderModuleInfo.codeSize = shaderCode.size();
		shaderModuleInfo.pCode = (const uint32_t*)shaderCode.data();

		VkShaderModule shaderModule;
		CHECK_VULKAN(vkCreateShaderModule(system.GetDevice(), &shaderModuleInfo, nullptr, &shaderModule), "Failed to create shader module!");
		system.DebugNameObject(shaderModule, VK_OBJECT_TYPE_SHADER_MODULE, "ShaderModule", WinUtils::GetJustFileName(filename));
		return shaderModule;
	}

	VkImageView CreateImageView(const VulkanSystem& system, VkImage image, uint32_t mipLevels, VkFormat format, VkImageAspectFlags aspectFlags, const std::string& debugName, VkImageViewType viewType, uint32_t numImages)
	{
		VkImageViewCreateInfo ImageViewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		ImageViewInfo.image = image;
		ImageViewInfo.viewType = viewType;
		ImageViewInfo.format = format;
		ImageViewInfo.subresourceRange.aspectMask = aspectFlags;
		ImageViewInfo.subresourceRange.baseMipLevel = 0;
		ImageViewInfo.subresourceRange.levelCount = mipLevels;
		ImageViewInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewInfo.subresourceRange.layerCount = numImages;;

		ImageViewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };	// Default bits - is this needed?

		VkImageView imageView;
		CHECK_VULKAN(vkCreateImageView(system.GetDevice(), &ImageViewInfo, nullptr, &imageView), "Failed to create image view!");
		system.DebugNameObject(imageView, VK_OBJECT_TYPE_IMAGE_VIEW, "ImageView", debugName);
		return imageView;
	}

	VkSampler CreateSampler(const VulkanSystem& system, uint32_t mipLevels, uint32_t maxAnisotropy, VkSamplerAddressMode addressMode, VkBorderColor borderColor, VkCompareOp compareOp, const std::string& debugName)
	{
		VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

		samplerInfo.addressModeU = addressMode;
		samplerInfo.addressModeV = addressMode;
		samplerInfo.addressModeW = addressMode;

		samplerInfo.anisotropyEnable = (maxAnisotropy > 0) ? VK_TRUE : VK_FALSE;
		samplerInfo.maxAnisotropy = (float)maxAnisotropy;
		samplerInfo.borderColor = borderColor;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = compareOp;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.minLod = 0;
		samplerInfo.maxLod = (float)mipLevels;
		samplerInfo.mipLodBias = 0;

		VkSampler textureSampler;
		CHECK_VULKAN(vkCreateSampler(system.GetDevice(), &samplerInfo, nullptr, &textureSampler), "Failed to create texture sampler!");
		system.DebugNameObject(textureSampler, VK_OBJECT_TYPE_SAMPLER, "Sampler", debugName);
		return textureSampler;
	}

	std::string GetModelFile(const std::string& projectName, const std::string& modelFile)
	{
		std::string searchdir;
		if (!projectName.empty())
			searchdir = projectName + '\\';
		return WinUtils::FindFile(searchdir + "models\\" + modelFile);
	}

	glm::vec3 CalculateFrontVector(float yaw, float pitch)
	{
		glm::vec3 front;
		front.x = cos(yaw) * cos(pitch);
		front.y = sin(pitch);
		front.z = sin(yaw) * cos(pitch);
		return glm::normalize(front);
	}
	glm::mat4 CalcViewMatrix(const glm::vec3& pos, float yaw, float pitch)
	{
		glm::vec3 front = CalculateFrontVector(yaw, pitch);
		const glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 right = glm::normalize(glm::cross(front, WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		glm::vec3 up = glm::normalize(glm::cross(right, front));

		return glm::lookAt(pos, pos + front, up);
	}
	void SetupProjectionMatrix(glm::mat4& projection, float aspectRatio, float fov, float zNear, float zFar)
	{
		projection = glm::perspective(glm::radians(fov), aspectRatio, zNear, zFar);
	}

	void EnableFillModeNonSolid(const VkPhysicalDeviceFeatures& deviceFeatures, VkPhysicalDeviceFeatures* requiredFeatures)
	{
		if (deviceFeatures.fillModeNonSolid == VK_FALSE)
			throw std::runtime_error("Line polygon mode not supported");

		requiredFeatures->fillModeNonSolid = VK_TRUE;
	}
	void EnableWideLines(const VkPhysicalDeviceFeatures& deviceFeatures, VkPhysicalDeviceFeatures * requiredFeatures)
	{
		if (deviceFeatures.wideLines == VK_FALSE)
			throw std::runtime_error("Wide lines not supported");

		requiredFeatures->wideLines = VK_TRUE;
	}
}

UnicodeString::UnicodeString(const std::string& utf8String)
{
	string = WinUtils::UTF16toUTF32(WinUtils::Widen(utf8String));
}

UnicodeString::UnicodeString(const std::wstring& utf16String)
{
	string = WinUtils::UTF16toUTF32(utf16String);
}

UnicodeString::UnicodeString(const std::u32string& utf32String)
{
	string = utf32String;
}

std::map<int, std::u32string> UnicodeString::strings;

UnicodeString::UnicodeString(int resourceID)
{
	Load(resourceID);
}
void UnicodeString::Load(int resourceID)
{
	auto pos = strings.find(resourceID);
	if (pos != strings.end())
		string = pos->second;
	else
	{
		string = WinUtils::UTF16toUTF32(WinUtils::LoadResourceString(resourceID));
		strings[resourceID] = string;
	}
}

uint32_t Attribs::GetStride(const std::vector<Attrib>& attribs)
{
	return std::accumulate(attribs.begin(), attribs.end(), 0, [](int total, const auto& attrib) { return total + FormatSize(attrib.format); });
}

uint32_t Attribs::FormatSize(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_R32G32B32_SFLOAT:
		return 3 * sizeof(float);
		break;
	case VK_FORMAT_R32G32_SFLOAT:
		return 2 * sizeof(float);
		break;
	case VK_FORMAT_R32_SFLOAT:
		return 1 * sizeof(float);
		break;
	case VK_FORMAT_R32_SINT:
		return 1 * sizeof(int);
		break;
	case VK_FORMAT_R32G32_UINT:
		return 2 * sizeof(unsigned int);
		break;		
	case VK_FORMAT_R8G8B8A8_UNORM:
		return 4 * sizeof(unsigned char);
		break;
	case VK_FORMAT_R8_UNORM:
		return 1 * sizeof(unsigned char);
		break;
	default:
		throw "error";
	}
}
