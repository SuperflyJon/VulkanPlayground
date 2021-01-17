
#include "stdafx.h"
#include "RenderPass.h"
#include "Common.h"
#include "SwapChain.h"
#include "System.h"

uint32_t RenderPass::AddColourAttachment(VkFormat format, const std::string& debugName, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkImageLayout initialLayout, VkImageLayout finalLayout)
{
	VkAttachmentReference colourAttachmentRef{};
	colourAttachmentRef.attachment = (uint32_t)renderAttachments.size();
	colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	CurrentSubPass().colourAttachments.push_back(colourAttachmentRef);

	VkAttachmentDescription colourAttachment{};
	colourAttachment.format = format;
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colourAttachment.loadOp = loadOp;
	colourAttachment.storeOp = storeOp;
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colourAttachment.initialLayout = initialLayout;
	colourAttachment.finalLayout = finalLayout;

	renderAttachments.push_back(colourAttachment);
	renderAttachmentsUseSwapChainImage.push_back(colourAttachment.finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	renderAttachmentsNames.push_back(debugName);

	VkClearValue clearValue;
	memcpy(clearValue.color.float32, glm::value_ptr(clearColour), sizeof(clearValue.color));
	clearColours.push_back(clearValue);

	return colourAttachmentRef.attachment;
}

void RenderPass::AddExistingColourAttachment(uint32_t previousAttachment)
{
	VkAttachmentReference colourAttachmentRef{};
	colourAttachmentRef.attachment = previousAttachment;
	colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	CurrentSubPass().colourAttachments.push_back(colourAttachmentRef);
}

uint32_t RenderPass::AddDepthAttachment(VkFormat depthBufferFormat)
{
	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = (uint32_t)renderAttachments.size();
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	CurrentSubPass().depthAttachments.push_back(depthAttachmentRef);

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthBufferFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	renderAttachments.push_back(depthAttachment);
	renderAttachmentsUseSwapChainImage.push_back(false);

	VkClearValue clearValue;
	clearValue.depthStencil = depthStencilClearValue;
	clearColours.push_back(clearValue);

	return depthAttachmentRef.attachment;
}

void RenderPass::AddInput(uint32_t attachment)
{
	VkAttachmentReference ref{ attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	CurrentSubPass().inputs.push_back(ref);
}

void RenderPass::Create(VulkanSystem &system, const std::string& debugName)
{
	std::vector<VkSubpassDescription> subPassDescs;

	for (auto& subPass : subpasses)
	{
		VkSubpassDescription subPassDesc{};
		subPassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		if (!subPass.colourAttachments.empty())
		{
			subPassDesc.colorAttachmentCount = (uint32_t)subPass.colourAttachments.size();
			subPassDesc.pColorAttachments = subPass.colourAttachments.data();
		}
		if (!subPass.depthAttachments.empty())
			subPassDesc.pDepthStencilAttachment = subPass.depthAttachments.data();

		if (!subPass.inputs.empty())
		{
			subPassDesc.inputAttachmentCount = (uint32_t)subPass.inputs.size();
			subPassDesc.pInputAttachments = subPass.inputs.data();
		}

		subPassDescs.push_back(subPassDesc);
	}

	std::vector<VkSubpassDependency> dependencies;

	VkSubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies.push_back(dependency);

	if (subpasses.size() > 1)
	{
		VkSubpassDependency subpassDependency;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		for (int i = 1; i < subpasses.size(); i++)
		{
			subpassDependency.srcSubpass = i - 1;
			subpassDependency.dstSubpass = i;

			dependencies.push_back(subpassDependency);
		}
	}

	VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount = (uint32_t)renderAttachments.size();
	renderPassInfo.pAttachments = renderAttachments.data();
	renderPassInfo.subpassCount = (uint32_t)subPassDescs.size();
	renderPassInfo.pSubpasses = subPassDescs.data();
	renderPassInfo.dependencyCount = (uint32_t)dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();
	CHECK_VULKAN(vkCreateRenderPass(system.GetDevice(), &renderPassInfo, nullptr, &renderPass), "Failed to create render pass!");
	system.DebugNameObject(renderPass, VK_OBJECT_TYPE_RENDER_PASS, "Renderpass", debugName);

	imageAvailable.Create(system, "Image available");
	renderFinished.Create(system, "Render finished");
}

void RenderPass::Tidy(VulkanSystem& system)
{
	displayBuffers.Tidy(system);
	imageAvailable.Tidy(system);
	renderFinished.Tidy(system);

	if (renderPass != nullptr)
	{
		vkDestroyRenderPass(system.GetDevice(), renderPass, nullptr);
		renderAttachments.clear();
		renderAttachmentsNames.clear();
		renderAttachmentsUseSwapChainImage.clear();

		for (auto& subPass : subpasses)
		{
			subPass.depthAttachments.clear();
			subPass.colourAttachments.clear();
		}
		subpasses.clear();
		renderPass = nullptr;
		AddNewSubPass();	// Reset to single empty entry
	}
}

void RenderPass::Begin(VkFramebuffer frameBuffer, VkCommandBuffer commandBuffer, const VkExtent2D& extent)
{
	VkRenderPassBeginInfo renderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = frameBuffer;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = extent;
	renderPassBeginInfo.clearValueCount = (uint32_t)clearColours.size();
	renderPassBeginInfo.pClearValues = clearColours.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderPass::End(VkCommandBuffer commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}

bool RenderPass::IsInputAttachment(uint32_t index) const
{
	for (auto it = subpasses.begin() + 1; it != subpasses.end(); it++)
	{
		if (std::any_of(it->inputs.begin(), it->inputs.end(), [index](const auto& input) { return (input.attachment == index); }))
			return true;
	}
	return false;
}

VkFormat RenderPass::GetAttachmentFormat(uint32_t index) const
{
	return renderAttachments[index].format;
}

void RenderPasses::Tidy(VulkanSystem& system)
{
	for (auto renderPass : renderPasses)
	{
		renderPass->Tidy(system);
		delete renderPass;
	}
	renderPasses.clear();

	if (offscreenRenderPass)
	{
		offscreenRenderPass->Tidy(system);
		delete offscreenRenderPass;
	}
	offscreenRenderPass = nullptr;
}

void RenderPass::SwitchOutputToAttachment()
{
	for (unsigned int attachment = 0; attachment < renderAttachments.size(); attachment++)
	{
		if (IsSwapChainImage(attachment))
			renderAttachments[attachment].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
}

RenderPass& RenderPasses::NewRenderPass()
{
	if (!renderPasses.empty())
	{	// Assume previous renderpass feeds into this one / make last renderpass the "output" one
		renderPasses.back()->SwitchOutputToAttachment();
	}

	RenderPass* pRenderPass = new RenderPass();
	renderPasses.emplace_back(pRenderPass);
	return *pRenderPass;
}

OffscreenRenderPass& RenderPasses::CreateOffscreenRenderPass(VkExtent2D extent)
{
	offscreenRenderPass = new OffscreenRenderPass(extent);
	return *offscreenRenderPass;
}

RenderPass& RenderPasses::GetScreenRenderPass()
{
	return *renderPasses.front();
}

void OffscreenRenderPass::CreateFrameBuffer(VulkanSystem& system, VkFormat depthBufferFormat)
{
	displayBuffers.CreateFrameBuffer(system, *this, nullptr, extent, depthBufferFormat, "OffscreenBuffer");
	auto& image = *displayBuffers.GetAttachmentImage(0).front();

	texture.SetImageView(image.GetImageView());
	texture.CreateSampler(system, "Offscreen");
	imageCache = &image;
}
