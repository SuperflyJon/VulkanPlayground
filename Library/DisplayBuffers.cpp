#include "stdafx.h"
#include "DisplayBuffers.h"
#include "RenderPass.h"
#include "SwapChain.h"
#include "System.h"

void Semaphore::Create(VulkanSystem& system, const std::string& debugName)
{
	if (semaphore == nullptr)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		CHECK_VULKAN(vkCreateSemaphore(system.GetDevice(), &semaphoreCreateInfo, nullptr, &semaphore), "Failed to create semaphore!");
		system.DebugNameObject(semaphore, VK_OBJECT_TYPE_SEMAPHORE, "Semaphore", debugName);
	}
}

void Semaphore::Tidy(VulkanSystem& system)
{
	if (semaphore != nullptr)
	{
		vkDestroySemaphore(system.GetDevice(), semaphore, nullptr);
		semaphore = nullptr;
	}
}

void DisplayBuffers::CreateFrameBuffer(VulkanSystem& system, RenderPass& renderPass, VkImageView swapChainImageView, const VkExtent2D& workingExtent, VkFormat depthBufferFormat, const std::string& debugName)
{
	extent = workingExtent;

	Buffer buffer;
	std::vector<VkImageView> bufAttachments;
	for (uint32_t index = 0; index < renderPass.GetNumAttacachments(); index++)
	{
		if (renderPass.IsDepthBuffer(index))
		{
			if (!depthImage.Created())
			{
				VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				bool depthBufferAttachment = renderPass.IsInputAttachment(index);
				if (depthBufferAttachment)
					usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
				depthImage.Create(system, depthBufferFormat, usage, extent.width, extent.height, 1, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImageAspect, "DepthBuffer");

				auto inc = system.GetScopedDebugOutputIncrement();
				system.GetBufMan().TansitionImageLayout(system, system.GetGraphicsQueuePool(), depthImage.GetImage(), 1, depthBufferFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

				if (depthBufferAttachment)
					attachmentImagesMap[index].push_back(&depthImage);
			}
			bufAttachments.push_back(depthImage.GetImageView());
		}
		else if (renderPass.IsSwapChainImage(index))
		{
			buffer.swapChainImageView = swapChainImageView;	// Remember so can be cleaned up later
			bufAttachments.push_back(swapChainImageView);
		}
		else
		{	// Create an attachment image
			ImageWithView* attachmentImage = new ImageWithView();
			VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			if (renderPass.IsInputAttachment(index))
				usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
			else
				usage |= VK_IMAGE_USAGE_SAMPLED_BIT;	// Assume we are going to read from an off-screen buffer

			attachmentImage->Create(system, renderPass.GetAttachmentFormat(index), usage, extent.width, extent.height, 1, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, renderPass.GetAttachmentDebugName(index));

			bufAttachments.push_back(attachmentImage->GetImageView());

			attachmentImagesMap[index].push_back(attachmentImage);
		}
	}

	VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferInfo.renderPass = renderPass.Get();
	framebufferInfo.width = extent.width;
	framebufferInfo.height = extent.height;
	framebufferInfo.layers = 1;
	framebufferInfo.attachmentCount = (uint32_t)bufAttachments.size();
	framebufferInfo.pAttachments = bufAttachments.data();
	CHECK_VULKAN(vkCreateFramebuffer(system.GetDevice(), &framebufferInfo, nullptr, &buffer.frameBuffer), "Failed to create framebuffer!");
	system.DebugNameObject(buffer.frameBuffer, VK_OBJECT_TYPE_FRAMEBUFFER, "Framebuffer", debugName);
	buffers.push_back(buffer);
}

void DisplayBuffers::FreeCmdBuffers(VulkanSystem& system)
{
	if (buffers.empty())
		return;

	//TODO: Ideally reuse a set number of buffers - i.e. no need to free buffers and recreate essentially the same ones?
	system.DeviceWaitIdle();

	for (auto& buffer : buffers)
	{
		if (buffer.commandBuffer != nullptr)
		{
			vkFreeCommandBuffers(system.GetDevice(), system.GetGraphicsQueuePool().GetPool(), 1, &buffer.commandBuffer);
			buffer.commandBuffer = nullptr;
		}
	}
}

void DisplayBuffers::CreateCmdBuffers(VulkanSystem& system, RenderPass& renderPass, std::function<void(VkCommandBuffer)> DrawFun, const std::string& debugName)
{
	FreeCmdBuffers(system);

	// Create command buffers
	std::vector<VkCommandBuffer> commandBuffers(buffers.size());

	VkCommandBufferAllocateInfo commandBufferAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	commandBufferAllocateInfo.commandPool = system.GetGraphicsQueuePool().GetPool();
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = (uint32_t)commandBuffers.size();
	CHECK_VULKAN(vkAllocateCommandBuffers(system.GetDevice(), &commandBufferAllocateInfo, commandBuffers.data()), "Failed to allocate command buffers!");
	for (uint32_t i = 0; i < commandBuffers.size(); i++)
	{
		buffers[i].commandBuffer = commandBuffers[i];

		std::stringstream ss;
		ss << "Command Buffer {" << i << "}";
		system.DebugNameObject(commandBuffers[i], VK_OBJECT_TYPE_COMMAND_BUFFER, ss.str(), debugName);
	}

	VkCommandBufferBeginInfo commandBufferBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	for (auto& buffer : buffers)
	{
		CHECK_VULKAN(vkBeginCommandBuffer(buffer.commandBuffer, &commandBufferBeginInfo), "BeginCommandBuffer failed!");

		renderPass.Begin(buffer.frameBuffer, buffer.commandBuffer, extent);
		DrawFun(buffer.commandBuffer);
		renderPass.End(buffer.commandBuffer);

		CHECK_VULKAN(vkEndCommandBuffer(buffer.commandBuffer), "Failed to record command buffer!");
	}
}

void DisplayBuffers::SubmitCommandBuffer(uint32_t buffNum, VkQueue queue, const std::vector<VkSemaphore>& waitSemaphores, const std::vector<VkSemaphore>& signalSemaphores, VkPipelineStageFlags waitStage)
{
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };

	std::vector<VkPipelineStageFlags> waitStages;
	if (!waitSemaphores.empty())
	{
		submitInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size();
		submitInfo.pWaitSemaphores = waitSemaphores.data();
		waitStages.resize(waitSemaphores.size(), waitStage);
		submitInfo.pWaitDstStageMask = waitStages.data();
	}
	if (buffers[buffNum].commandBuffer != nullptr)
	{
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &buffers[buffNum].commandBuffer;
	}
	if (!signalSemaphores.empty())
	{
		submitInfo.signalSemaphoreCount = (uint32_t)signalSemaphores.size();
		submitInfo.pSignalSemaphores = signalSemaphores.data();
	}
	CHECK_VULKAN(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE), "Failed to submit draw command");
}

void DisplayBuffers::Tidy(VulkanSystem& system)
{
	for (auto& buffer : buffers)
	{
		if (buffer.frameBuffer != nullptr)
		{
			vkDestroyFramebuffer(system.GetDevice(), buffer.frameBuffer, nullptr);
		}
		if (buffer.swapChainImageView != nullptr)
		{
			vkDestroyImageView(system.GetDevice(), buffer.swapChainImageView, nullptr);
		}
	}
	FreeCmdBuffers(system);

	buffers.clear();

	for (auto item : attachmentImagesMap)
	{
		for (auto image : item.second)
		{
			if (image->Created() && image != &depthImage)
			{
				image->Tidy(system);
				delete image;
			}
		}
	}
	attachmentImagesMap.clear();

	if (depthImage.Created())
	{
		depthImage.Tidy(system);
	}
}
