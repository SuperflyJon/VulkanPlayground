#pragma once

#include "Common.h"

class VulkanSystem;
class Image;
struct PixelData;

class BufferBase
{
public:
	BufferBase() : created(false), deviceMemory(nullptr)
	{}
	BufferBase(const BufferBase& other) = delete;
	BufferBase& operator=(const BufferBase& other) = delete;

	BufferBase(BufferBase&& other) noexcept
		: created(other.created), deviceMemory(other.deviceMemory)
	{
		other.created = false;
	}
	BufferBase& operator=(BufferBase&& other) noexcept
	{
		created = other.created;
		deviceMemory = other.deviceMemory;
		other.created = false;
		return *this;
	}

	void DestroyBuffer(VulkanSystem& system);

	bool Created() const { return created; }
	VkDeviceMemory GetDeviceMemory() const { return deviceMemory; }

protected:
	virtual void FreeBufferInternal(VulkanSystem& system) = 0;

	bool created;
	VkDeviceMemory deviceMemory;
};

class Buffer : public BufferBase
{
public:
	explicit Buffer() : buffer(nullptr), bufferSize(0), bufferInfo{} {}

	void* Map(VkDevice device);
	void CopyData(VkDevice device, const void* data, VkDeviceSize dataSize);
	void CopyDataRanges(VkDevice device, uint32_t numRanges, VkMappedMemoryRange* memoryRanges) const;
	void Create(VulkanSystem& system, VkDeviceSize _size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const std::string& debugName, std::set<uint32_t> queueIndicies = {});

	VkBuffer& GetBuffer() { return buffer; }
	VkDeviceSize GetBufferSize() const { return bufferSize; }

	void Bind(VkCommandBuffer commandBuffer);

protected:
	void FreeBufferInternal(VulkanSystem& system) override;

private:
	VkBuffer buffer;
	VkDeviceSize bufferSize;
	VkBufferCreateInfo bufferInfo;
};

class QueuePool : public ITidy
{
public:
	QueuePool() : pool(nullptr), queue(nullptr)
	{}
	void Setup(VulkanSystem& system, uint32_t queueFamily, const std::string& debugName);

	void Tidy(VulkanSystem& system) override;

	VkCommandPool GetPool() const { return pool; }
	VkQueue GetQueue() const { return queue; }

private:
	VkCommandPool pool;
	VkQueue queue;
};

class SingleCommand
{
public:
	SingleCommand(const VulkanSystem& system, QueuePool commandQueuePool, const std::string& debugName) : queuePool(commandQueuePool), buffer(nullptr)
	{
		Begin(system, debugName);
	}
	void Run(const VulkanSystem& system);
	
	VkCommandBuffer GetBuffer() const { return buffer; }

protected:
	VkCommandBuffer Begin(const VulkanSystem& system, const std::string& debugName);

private:
	QueuePool queuePool;
	VkCommandBuffer buffer;
};

class BufferManager : public ITidy
{
public:
	BufferManager();
	void Setup(VulkanSystem& system);

	void CreateGpuImage(VulkanSystem& system, Image& image, const PixelData& pd, VkImageUsageFlagBits usage, VkFormat format, uint32_t numFaces, VkImageViewType imageType, const std::string& debugName);
	void CreateGpuBuffer(VulkanSystem& system, Buffer& buffer, const void* data, VkDeviceSize bufferSize, VkBufferUsageFlagBits usage, const std::string& debugName);
	void CopyToStagingBuffer(VulkanSystem& system, const void* data, VkDeviceSize bufferSize);

	void Copy(const VulkanSystem& system, Buffer& source, Buffer& dest, VkDeviceSize bufferSize);
	void GenerateMipmaps(const VulkanSystem& system,  VkImage image, uint32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	void CopyBufferToImage(const VulkanSystem& system, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void Tidy(VulkanSystem& system) override;

	static void TansitionImageLayout(const VulkanSystem& system, const QueuePool& queuePool, VkImage image, uint32_t mipLevels, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t numFaces = 1);

	QueuePool& GetGraphicsQueuePool() { return graphicsQueuePool; }

private:
	QueuePool transferQueuePool;
	QueuePool graphicsQueuePool;

	std::vector<Buffer*> buffersToTidy;

	VkDeviceSize stagingBufferSize;
	Buffer stagingBuffer;
};
