#include "stdafx.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "Image.h"
#include "System.h"

SwapChain::SwapChain()
	: swapChain(nullptr), workingExtent{}, imageCount(0)
{
	swapChainImageFormat.format = VK_FORMAT_UNDEFINED;
}

void SwapChain::Tidy(VulkanSystem& system)
{
	if (swapChain != nullptr)
	{
		system.DeviceWaitIdle();
		vkDestroySwapchainKHR(system.GetDevice(), swapChain, nullptr);
		swapChain = nullptr;
	}
}

void SwapChain::Create(VulkanSystem &system, VkSurfaceKHR windowSurface, uint32_t width, uint32_t height, bool vSync, const std::string& debugName)
{
	swapChainDetails = GetSwapChainSupportDetails(system.GetPhysicalDevice(), windowSurface);

	swapChainImageFormat = ChooseSwapSurfaceFormat(swapChainDetails.formats);

	VkExtent2D actualExtent{ width, height };
	if (swapChainDetails.capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
	{
		actualExtent.width = std::clamp(actualExtent.width, swapChainDetails.capabilities.minImageExtent.width, swapChainDetails.capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, swapChainDetails.capabilities.minImageExtent.height, swapChainDetails.capabilities.maxImageExtent.height);
	}
	workingExtent = actualExtent;

	uint32_t minImageCount = VulkanPlayground::bufferCount;
	minImageCount = std::clamp(minImageCount, swapChainDetails.capabilities.minImageCount, swapChainDetails.capabilities.maxImageCount);	// Clamp to supported range

	VkSwapchainCreateInfoKHR swapchainInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainInfo.surface = windowSurface;
	swapchainInfo.minImageCount = minImageCount;
	swapchainInfo.imageFormat = swapChainImageFormat.format;
	swapchainInfo.imageColorSpace = swapChainImageFormat.colorSpace;
	swapchainInfo.imageExtent = workingExtent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	std::vector<uint32_t> queueFamilyIndices{ (uint32_t)system.GetQueueIndicies().graphicsFamily, (uint32_t)system.GetQueueIndicies().presentFamily };	// Needs to stay in scope
	if (system.GetQueueIndicies().graphicsFamily == system.GetQueueIndicies().presentFamily)
	{
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	else
	{
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainInfo.queueFamilyIndexCount = (uint32_t)queueFamilyIndices.size();
		swapchainInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	}

	swapchainInfo.preTransform = swapChainDetails.capabilities.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode = ChooseSwapPresentMode(swapChainDetails.presentModes, vSync);
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

	CHECK_VULKAN_THROW(vkCreateSwapchainKHR(system.GetDevice(), &swapchainInfo, nullptr, &swapChain), "Failed to create swap chain!");

	std::stringstream ss;
	ss << debugName << " {" << workingExtent.width << ", " << workingExtent.height << "}]";
	system.DebugNameObject(swapChain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "SwapChain", ss.str());
	auto inc = system.GetScopedDebugOutputIncrement();

	// Get a handle to the created queues
	graphicsQueue = system.GetGraphicsQueuePool().GetQueue();
	vkGetDeviceQueue(system.GetDevice(), system.GetQueueIndicies().presentFamily, 0, &presentQueue);
	system.DebugNameObject(presentQueue, VK_OBJECT_TYPE_QUEUE, "Queue", debugName);
}

void SwapChain::CreateFrameBuffers(VulkanSystem &system, RenderPasses& renderPasses, VkFormat depthBufferFormat, const std::string& debugName)
{
	// Get the swap chain images (NB. may be more than we asked for (unlikely))
	std::vector<VkImage> swapChainImages;
	CHECK_VULKAN(vkGetSwapchainImagesKHR(system.GetDevice(), swapChain, &imageCount, nullptr), "Failed to get swapchain images");
	swapChainImages.resize(imageCount);
	CHECK_VULKAN(vkGetSwapchainImagesKHR(system.GetDevice(), swapChain, &imageCount, swapChainImages.data()), "Failed to get swapchain images");

	for (auto renderPass : renderPasses.GetRenderPasses())
	{
		// Create RenderPass
		renderPass->Create(system, debugName);

		// Create Frame Buffers
		char bufName{ 'A' };
		for (auto image : swapChainImages)
		{
			std::stringstream ss;
			ss << debugName << " {" << bufName << "}";
			std::string debugString = ss.str();
			system.DebugNameObject(image, VK_OBJECT_TYPE_IMAGE, "DisplayBuffer", debugString);
			auto inc = system.GetScopedDebugOutputIncrement();

			VkImageView imageView = VulkanPlayground::CreateImageView(system, image, 1, swapChainImageFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, debugString);
			renderPass->CreateFrameBuffer(system, imageView, workingExtent, depthBufferFormat, debugString);
			bufName++;
		}
	}
	auto offscreenRenderPass = renderPasses.GetOffscreenRenderPass();
	if (offscreenRenderPass)
	{
		offscreenRenderPass->Create(system, "Offscreen");
		offscreenRenderPass->CreateFrameBuffer(system, depthBufferFormat);
	}
}

SwapChainSupportDetails SwapChain::GetSwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	details.formats.resize(formatCount);
	if (formatCount > 0)
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	details.presentModes.resize(presentModeCount);
	if (presentModeCount > 0)
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());

	return details;
}

bool SwapChain::DrawFrame(VulkanSystem& system, RenderPasses &renderPasses)
{
	system.DebugInsertLabel(system.GetGraphicsQueuePool().GetQueue(), "DrawFrame", { 1.0f, 0.0f, 0.0f });

	VkSemaphore waitSemaphore = renderPasses.GetInitialWaitSemaphore();
	std::vector<VkSemaphore> waitSemaphores{ waitSemaphore };

	RenderPass* offscreenRenderPass = renderPasses.GetOffscreenRenderPass();
	if (offscreenRenderPass)
	{
		auto extraWait = offscreenRenderPass->SubmitCommandBuffer(0, graphicsQueue, {});
		waitSemaphores.push_back(extraWait);
	}

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(system.GetDevice(), swapChain, VulkanPlayground::NO_TIMEOUT, waitSemaphore, VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		return false;
	if (!CHECK_VULKAN(result, "Failed to acquire swap chain image!"))
		return false;

	for (auto renderPass : renderPasses.GetRenderPasses())
	{
		waitSemaphore = renderPass->SubmitCommandBuffer(imageIndex, graphicsQueue, waitSemaphores);
		waitSemaphores = { waitSemaphore };
	}

	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &waitSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &imageIndex;
	result = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		return false;
	CHECK_VULKAN(result, "Failed to present swap chain image!");

	return true;
}

VkSurfaceFormatKHR SwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats.front().format == VK_FORMAT_UNDEFINED)
	{
		if (VulkanPlayground::showObjectCreationMessages)
			std::cout << "Surface format {RGBA_UNORM, SRGB}";

		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	if (std::any_of(availableFormats.begin(), availableFormats.end(), [](auto& format) { return (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR); }))
	{
		if (VulkanPlayground::showObjectCreationMessages)
			std::cout << "Surface format {RGBA_UNORM, SRGB}";

		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	if (VulkanPlayground::showObjectCreationMessages)
		std::cout << "Surface format {" << availableFormats.front().format << ", " << availableFormats.front().colorSpace << "}";

	return availableFormats.front();
}

VkPresentModeKHR SwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool vSync)
{
	if (!vSync)
	{
		if (std::find(availablePresentModes.begin(), availablePresentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != availablePresentModes.end())
		{
			if (VulkanPlayground::showObjectCreationMessages)
				std::cout << " : [mailbox present mode]\n";

			return VK_PRESENT_MODE_MAILBOX_KHR;
		}

		if (std::find(availablePresentModes.begin(), availablePresentModes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != availablePresentModes.end())
		{
			if (VulkanPlayground::showObjectCreationMessages)
				std::cout << " : [immediate present]\n";

			return VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}
	if (VulkanPlayground::showObjectCreationMessages)
		std::cout << " : [FIFO present mode]\n";

	return VK_PRESENT_MODE_FIFO_KHR;
}
