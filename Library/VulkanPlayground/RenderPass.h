#pragma once

#include "DisplayBuffers.h"
#include "Image.h"

class VulkanSystem;
class BufferManager;
typedef std::vector<ImageWithView*> ImageWithViewList;

class RenderPass : public ITidy
{
	struct SubPass
	{
		std::vector<VkAttachmentReference> colourAttachments;
		std::vector<VkAttachmentReference> depthAttachments;
		std::vector<VkAttachmentReference> inputs;	// From previous subpass
	};

public:
	RenderPass()
		: renderPass(nullptr)
	{
		bufName = 'A';
		AddNewSubPass();	// Need at least one subpass
		depthStencilClearValue = { 1.0f, 0 };
	}
	void AddNewSubPass() { subpasses.emplace_back(); }
	void AddInput(uint32_t attachment);
	SubPass& CurrentSubPass() { return subpasses.back(); }

	uint32_t AddColourAttachment(VkFormat format, const std::string& debugName, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	uint32_t AddIntermidiateColourAttachment(VkFormat format, const std::string& debugName, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
							{ return AddColourAttachment(format, debugName, loadOp, storeOp, initialLayout, finalLayout); }
	void AddExistingColourAttachment(uint32_t previousAttachment);
	uint32_t AddDepthAttachment(VkFormat depthBufferFormat);

	bool HasDepthBuffer(uint32_t subpass = 0) const { return !subpasses[subpass].depthAttachments.empty(); }
	bool IsDepthBuffer(uint32_t index, uint32_t subpass = 0) const { return HasDepthBuffer(subpass) && index == subpasses[subpass].depthAttachments.front().attachment; }
	void SetDepthStencilClearValue(VkClearDepthStencilValue value) { depthStencilClearValue = value; }

	bool IsSwapChainImage(uint32_t index) const { return renderAttachmentsUseSwapChainImage[index]; }
	bool IsInputAttachment(uint32_t index) const;
	VkFormat GetAttachmentFormat(uint32_t index) const;

	void SwitchOutputToAttachment();
	const VkAttachmentDescription &GetAttacachment(uint32_t attahcmentNum) const { return renderAttachments[attahcmentNum]; }

	uint32_t GetNumAttacachments() const { return (uint32_t)renderAttachments.size(); }
	uint32_t GetNumColourAttacachments(uint32_t subpass = 0) const { return (uint32_t)subpasses[subpass].colourAttachments.size(); }

	void Create(VulkanSystem &system, const std::string& debugName);
	VkRenderPass Get() const { return renderPass; }
	void Tidy(VulkanSystem& system) override;

	void Begin(VkFramebuffer frameBuffer, VkCommandBuffer commandBuffer, const VkExtent2D& extent);
	static void End(VkCommandBuffer commandBuffer);

	void SetClearColour(const glm::vec4& val) { clearColour = val; }

	void CreateCmdBuffers(VulkanSystem& system, std::function<void(VkCommandBuffer)> DrawFun, const std::string& debugName)
	{
		displayBuffers.CreateCmdBuffers(system, *this, DrawFun, debugName);
	}
	void CreateFrameBuffer(VulkanSystem& system, VkImageView imageView, const VkExtent2D& extent, VkFormat depthBufferFormat, const std::string& debugName)
	{
		displayBuffers.CreateFrameBuffer(system, *this, imageView, extent, depthBufferFormat, debugName);
	}
	const ImageWithViewList& GetAttachmentImage(uint32_t index) { return displayBuffers.GetAttachmentImage(index); }

	VkSemaphore SubmitCommandBuffer(uint32_t imageIndex, VkQueue queue, const std::vector<VkSemaphore>& waitSemaphores, VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
	{
		displayBuffers.SubmitCommandBuffer(imageIndex, queue, waitSemaphores, { renderFinished.get() }, waitStage);
		return renderFinished.get();
	}
	VkSemaphore GetWaitSemaphore() const { return imageAvailable.get(); }

	DisplayBuffers& GetDisplayBuffers() { return displayBuffers; }

	std::string GetAttachmentDebugName(uint32_t index)
	{
		std::stringstream ss;
		ss << "Attachment " << renderAttachmentsNames[index] << " " << bufName++;
		return ss.str();
	}

private:
	char bufName;

protected:
	DisplayBuffers displayBuffers;

private:
	VkRenderPass renderPass;

	std::vector<VkAttachmentDescription> renderAttachments;
	std::vector<std::string> renderAttachmentsNames;
	std::vector<bool> renderAttachmentsUseSwapChainImage;
	std::vector<SubPass> subpasses;

	glm::vec4 clearColour;
	std::vector<VkClearValue> clearColours;

	VkClearDepthStencilValue depthStencilClearValue;

	Semaphore imageAvailable;
	Semaphore renderFinished;
};

class OffscreenRenderPass : public RenderPass
{
public:
	explicit OffscreenRenderPass(VkExtent2D offScreenExtent) : imageCache(nullptr), extent(offScreenExtent)
	{
	}

	void CreateFrameBuffer(VulkanSystem& system, VkFormat depthBufferFormat);

	TextureBase& GetBuffer() { return texture; }
	VkExtent2D GetExtent() const { return extent; }

private:
	TextureBase texture;
	ImageWithView* imageCache;
	VkExtent2D extent;
};

class RenderPasses : public ITidy
{
public:
	void Tidy(VulkanSystem& system) override;
	RenderPass& NewRenderPass();
	OffscreenRenderPass& CreateOffscreenRenderPass(VkExtent2D extent);
	RenderPass& GetScreenRenderPass();
	uint32_t size() const { return (uint32_t)renderPasses.size(); }

	void CreateCmdBuffers(uint32_t pass, VulkanSystem& system, std::function<void(VkCommandBuffer)> DrawFun, const std::string& debugName)
	{
		renderPasses[pass]->CreateCmdBuffers(system, DrawFun, debugName);
	}
	VkSemaphore SubmitCommandBuffer(uint32_t pass, uint32_t imageIndex, VkQueue queue, const std::vector<VkSemaphore>& waitSemaphores, VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
	{
		return renderPasses[pass]->SubmitCommandBuffer(imageIndex, queue, waitSemaphores, waitStage);
	}
	VkSemaphore GetInitialWaitSemaphore()
{
		return renderPasses[0]->GetWaitSemaphore();
	}

	std::vector<RenderPass*> GetRenderPasses() { return renderPasses; }
	OffscreenRenderPass* GetOffscreenRenderPass() { return offscreenRenderPass; }

private:
	std::vector<RenderPass*> renderPasses;
	OffscreenRenderPass* offscreenRenderPass = nullptr;
};
