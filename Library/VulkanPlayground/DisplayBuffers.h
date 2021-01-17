#pragma once

class VulkanSystem;
class RenderPass;
class RenderPasses;
class ImageWithView;
typedef std::vector<ImageWithView*> ImageWithViewList;
class BufferManager;
class ImageWithView;

#include "Image.h"
#include "Common.h"

class Semaphore : public ITidy
{
public:
	Semaphore() : semaphore(nullptr) {}
	void Create(VulkanSystem& system, const std::string& debugName);
	void Tidy(VulkanSystem& system) override;

	VkSemaphore get() const { return semaphore; }

private:
	VkSemaphore semaphore;
};

class DisplayBuffers : public ITidy
{
	struct Buffer
	{
		VkCommandBuffer commandBuffer = nullptr;
		VkFramebuffer frameBuffer = nullptr;
		VkImageView swapChainImageView = nullptr;	// Remember so can be cleaned up later
	};

public:
	DisplayBuffers() : extent{}, depthImageAspect(VK_IMAGE_ASPECT_DEPTH_BIT)
	{}

	void CreateFrameBuffer(VulkanSystem& system, RenderPass& renderPass, VkImageView imageView, const VkExtent2D& extent, VkFormat depthBufferFormat, const std::string& debugName);
	void CreateCmdBuffers(VulkanSystem& system, RenderPass& renderPass, std::function<void(VkCommandBuffer)> DrawFun, const std::string& debugName);
	void SubmitCommandBuffer(uint32_t buffNum, VkQueue queue, const std::vector<VkSemaphore>& waitSemaphores, const std::vector<VkSemaphore>& signalSemaphores, VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	void FreeCmdBuffers(VulkanSystem& system);
	void Tidy(VulkanSystem& system) override;

	void SetDepthImageAspect(VkImageAspectFlagBits value) { depthImageAspect = value; }
	const ImageWithViewList& GetAttachmentImage(uint32_t index) { return attachmentImagesMap[index]; }

private:
	VkExtent2D extent;
	std::vector<Buffer> buffers;

	std::map<uint32_t, ImageWithViewList> attachmentImagesMap;

	ImageWithView depthImage;
	VkImageAspectFlagBits depthImageAspect;
};
