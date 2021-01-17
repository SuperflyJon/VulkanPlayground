#include "stdafx.h"
#include "Buffers.h"
#include "System.h"
#include "Common.h"
#include "Image.h"
#include "PixelData.h"

void Buffer::Bind(VkCommandBuffer commandBuffer)
{
	if ((bufferInfo.usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer, VulkanPlayground::zeroOffset);
	else if ((bufferInfo.usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) == VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
		vkCmdBindIndexBuffer(commandBuffer, buffer, 0, VK_INDEX_TYPE_UINT32);
}

void Buffer::CopyData(VkDevice device, const void* data, VkDeviceSize dataSize)
{
	if (!created)
		throw std::runtime_error("Attempt to copy data to unallocated buffer!");

	void* memory;
	CHECK_VULKAN(vkMapMemory(device, deviceMemory, 0, dataSize, 0, &memory), "Failed to map memory!");
	memcpy(memory, data, dataSize);
	vkUnmapMemory(device, deviceMemory);
}

void* Buffer::Map(VkDevice device)
{
	if (!created)
		throw std::runtime_error("Attempt to map unallocated buffer!");

	void* memory;
	CHECK_VULKAN(vkMapMemory(device, deviceMemory, 0, bufferSize, 0, &memory), "Failed to map memory!");
	return memory;
}

void Buffer::CopyDataRanges(VkDevice device, uint32_t numRanges, VkMappedMemoryRange* memoryRanges) const
{
	if (!created)
		throw std::runtime_error("Attempt to copy data to unallocated buffer!");

	vkFlushMappedMemoryRanges(device, (uint32_t)numRanges, memoryRanges);
}

void Buffer::Create(VulkanSystem& system, VkDeviceSize _size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const std::string& debugName, std::set<uint32_t> queueIndicies)
{
	DestroyBuffer(system);

	bufferSize = _size;
	memset(&bufferInfo, 0, sizeof(bufferInfo));
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = usage;

	std::vector<uint32_t> vecQueueData(queueIndicies.begin(), queueIndicies.end());
	bufferInfo.sharingMode = (vecQueueData.size() > 1) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.queueFamilyIndexCount = (uint32_t)vecQueueData.size();
	bufferInfo.pQueueFamilyIndices = vecQueueData.data();

	CHECK_VULKAN(vkCreateBuffer(system.GetDevice(), &bufferInfo, nullptr, &buffer), "Failed to create vertex buffer!");
	system.DebugNameObject(buffer, VK_OBJECT_TYPE_BUFFER, "Buffer", debugName);
	auto inc = system.GetScopedDebugOutputIncrement();

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(system.GetDevice(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memRequirements.size;
	allocateInfo.memoryTypeIndex = system.FindMemoryType(memRequirements.memoryTypeBits, properties);

	CHECK_VULKAN(vkAllocateMemory(system.GetDevice(), &allocateInfo, nullptr, &deviceMemory), "Failed to allocate buffer memory");
	CHECK_VULKAN(vkBindBufferMemory(system.GetDevice(), buffer, deviceMemory, 0), "Failed to bind buffer");
	system.DebugNameObject(deviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "BufferMem", debugName);

	created = true;
}

void Buffer::FreeBufferInternal(VulkanSystem& system)
{
	vkDestroyBuffer(system.GetDevice(), buffer, nullptr);
}

BufferManager::BufferManager()
	: stagingBufferSize(0)
{
}

void QueuePool::Setup(VulkanSystem& system, uint32_t queueFamily, const std::string& debugName)
{
	vkGetDeviceQueue(system.GetDevice(), queueFamily, 0, &queue);
	system.DebugNameObject(queue, VK_OBJECT_TYPE_QUEUE, "Queue", debugName);

	VkCommandPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	poolInfo.queueFamilyIndex = queueFamily;
	CHECK_VULKAN(vkCreateCommandPool(system.GetDevice(), &poolInfo, nullptr, &pool), "Failed to create command pool!");
	system.DebugNameObject(pool, VK_OBJECT_TYPE_COMMAND_POOL, "Command pool", debugName);
}

void QueuePool::Tidy(VulkanSystem& system)
{
	if (pool != nullptr)
	{
		vkDestroyCommandPool(system.GetDevice(), pool, nullptr);
		pool = nullptr;
	}
}

void BufferManager::Setup(VulkanSystem& system)
{
	transferQueuePool.Setup(system, system.GetQueueIndicies().transferFamily, "TransferPool");
	graphicsQueuePool.Setup(system, system.GetQueueIndicies().graphicsFamily, "GraphicsPool");
}

void BufferManager::Copy(const VulkanSystem& system, Buffer& source, Buffer& dest, VkDeviceSize bufferSize)
{
	SingleCommand command(system, transferQueuePool, "Copy Buffer");

	VkBufferCopy copyRegion{};
	copyRegion.size = bufferSize;
	vkCmdCopyBuffer(command.GetBuffer(), source.GetBuffer(), dest.GetBuffer(), 1, &copyRegion);

	command.Run(system);
}

void BufferManager::GenerateMipmaps(const VulkanSystem& system, VkImage image, uint32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	SingleCommand command(system, graphicsQueuePool, "GenerateMipMaps");

	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 0; i < mipLevels - 1; i++)
	{
		barrier.subresourceRange.baseMipLevel = i;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(command.GetBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth / 2, mipHeight / 2, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i + 1;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(command.GetBuffer(), image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(command.GetBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		if (mipWidth > 1)
			mipWidth /= 2;
		if (mipHeight > 1)
			mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(command.GetBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	command.Run(system);
}

void BufferManager::CreateGpuImage(VulkanSystem& system, Image& image, const PixelData& pd, VkImageUsageFlagBits usage, VkFormat format, uint32_t numFaces, VkImageViewType imageType, const std::string& debugName)
{
	bool ktxImage = (numFaces > 0);
	if (!ktxImage)
		numFaces = 1;
	image.Create(system, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | usage, pd.width, pd.height, pd.mipLevels, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, debugName, { system.GetQueueIndicies().transferFamily, system.GetQueueIndicies().graphicsFamily }, numFaces, imageType);
	auto inc = system.GetScopedDebugOutputIncrement(2);

	uint32_t depth = 1;
	if (imageType == VK_IMAGE_VIEW_TYPE_3D)
	{	// Just a single 3d image volume with the set number of depth images
		depth = numFaces;
		numFaces = 1;
	}
	TansitionImageLayout(system, transferQueuePool, image.GetImage(), pd.mipLevels, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, numFaces);
	CopyToStagingBuffer(system, pd.pixels, pd.size);

	if (!ktxImage)
	{
		CopyBufferToImage(system, stagingBuffer.GetBuffer(), image.GetImage(), (uint32_t)pd.width, (uint32_t)pd.height);

		if (pd.mipLevels != 1)
		{
			GenerateMipmaps(system, image.GetImage(), pd.width, pd.height, pd.mipLevels);
			return;
		}
	}
	else
	{
		// Setup buffer copy regions for each face including all of it's miplevels
		std::vector<VkBufferImageCopy> bufferCopyRegions;
		uint32_t offset = 0;

		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.depth = depth;
		for (uint32_t face = 0; face < numFaces; face++)
		{
			for (uint32_t level = 0; level < pd.mipLevels; level++)
			{
				bufferCopyRegion.imageSubresource.mipLevel = level;
				bufferCopyRegion.imageSubresource.baseArrayLayer = face;
				bufferCopyRegion.bufferOffset = offset;
				bufferCopyRegion.imageExtent.width = pd.GetCubeFaceLevelWidth(face, level);
				bufferCopyRegion.imageExtent.height = pd.GetCubeFaceLevelHeight(face, level);

				bufferCopyRegions.push_back(bufferCopyRegion);

				// Increase offset into staging buffer for next level / face
				offset += pd.GetCubeFaceLevelSize(face, level);
			}
		}

		SingleCommand command(system, transferQueuePool, "CopyBufferToImage");
		vkCmdCopyBufferToImage(command.GetBuffer(), stagingBuffer.GetBuffer(), image.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)bufferCopyRegions.size(), bufferCopyRegions.data());
		command.Run(system);
	}
	TansitionImageLayout(system, graphicsQueuePool, image.GetImage(), pd.mipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, numFaces);
}

void BufferManager::TansitionImageLayout(const VulkanSystem& system, const QueuePool& queuePool, VkImage image, uint32_t mipLevels, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t numFaces)
{
	SingleCommand barrierCmd(system, queuePool, "TansitionImageLayout");
	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	if (numFaces > 1)
		barrier.subresourceRange.layerCount = numFaces;
	else
		barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags srcStage, dstStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
		throw std::runtime_error("Unsupport layout transition!");

	vkCmdPipelineBarrier(barrierCmd.GetBuffer(), srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	barrierCmd.Run(system);
}

void BufferManager::CopyBufferToImage(const VulkanSystem& system, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	SingleCommand copyCmd(system, transferQueuePool, "CopyBufferToImage");
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };
	vkCmdCopyBufferToImage(copyCmd.GetBuffer(), buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	copyCmd.Run(system);
}

void BufferManager::CopyToStagingBuffer(VulkanSystem& system, const void* data, VkDeviceSize bufferSize)
{
	if (bufferSize > stagingBufferSize)
	{
		if (stagingBufferSize > 0)
			stagingBuffer.DestroyBuffer(system);

		auto inc = system.GetScopedDebugOutputIncrement();
		stagingBuffer.Create(system, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Staging Buffer");
		stagingBufferSize = bufferSize;
	}
	stagingBuffer.CopyData(system.GetDevice(), data, bufferSize);
}

void BufferManager::CreateGpuBuffer(VulkanSystem& system, Buffer& buffer, const void* data, VkDeviceSize bufferSize, VkBufferUsageFlagBits usage, const std::string& debugName)
{
	if (!buffer.Created())
	{
		buffer.Create(system, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, debugName, { system.GetQueueIndicies().transferFamily, system.GetQueueIndicies().graphicsFamily });
		buffersToTidy.push_back(&buffer);

		CopyToStagingBuffer(system, data, bufferSize);
		Copy(system, stagingBuffer, buffer, bufferSize);
	}
}

void BufferManager::Tidy(VulkanSystem& system)
{
	if (stagingBufferSize > 0)
	{
		stagingBuffer.DestroyBuffer(system);
		stagingBufferSize = 0;
	}

	for (auto pBuffer : buffersToTidy)
		pBuffer->DestroyBuffer(system);

	transferQueuePool.Tidy(system);
	graphicsQueuePool.Tidy(system);
}

void BufferBase::DestroyBuffer(VulkanSystem& system)
{
	if (created)
	{
		FreeBufferInternal(system);
		vkFreeMemory(system.GetDevice(), deviceMemory, nullptr);
		created = false;
	}
}

VkCommandBuffer SingleCommand::Begin(const VulkanSystem& system, const std::string& debugName)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandPool = queuePool.GetPool();
	commandBufferAllocateInfo.commandBufferCount = 1;
	CHECK_VULKAN(vkAllocateCommandBuffers(system.GetDevice(), &commandBufferAllocateInfo, &buffer), "Failed to allocate command buffers!");
	system.DebugNameObject(buffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "SingleCommandBuffer", debugName);

	VkCommandBufferBeginInfo commandBufferBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	CHECK_VULKAN(vkBeginCommandBuffer(buffer, &commandBufferBeginInfo), "BeginCommandBuffer failed!");

	return buffer;
}

void SingleCommand::Run(const VulkanSystem& system)
{
	vkEndCommandBuffer(buffer);

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &buffer;
	CHECK_VULKAN(vkQueueSubmit(queuePool.GetQueue(), 1, &submitInfo, VK_NULL_HANDLE), "Failed to submit copy command");
	CHECK_VULKAN(vkQueueWaitIdle(queuePool.GetQueue()), "QueueWaitIdle failed!");	//TODO: Are fences better than blocking here...

	vkFreeCommandBuffers(system.GetDevice(), queuePool.GetPool(), 1, &buffer);
}
