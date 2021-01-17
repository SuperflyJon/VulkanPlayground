#pragma once

#include "System.h"

template <class UBO_DATA>
class UBO : public ITidy
{
public:
	void Create(VulkanSystem& system, const std::string& debugName)
	{
		uniformBuffer.Create(system, GetDataSize(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, debugName);
	}
	void Tidy(VulkanSystem& system) override
	{
		uniformBuffer.DestroyBuffer(system);
	}
	void CopyToDevice(const VulkanSystem& system)
	{
		uniformBuffer.CopyData(system.GetDevice(), &data, uniformBuffer.GetBufferSize());
	}

	Buffer& GetBuffer() { return uniformBuffer; }
	VkDeviceSize GetDataSize() { return sizeof(data); }

	UBO_DATA& GetData() { return data; }
	UBO_DATA& operator() () { return GetData(); }

private:
	UBO_DATA data;
	Buffer uniformBuffer;
};

template <class UBO_DATA>
class DynamicUBO : public ITidy
{
public:
	void Create(VulkanSystem& system, uint32_t dataSize, uint32_t numItems, const std::string& debugName)
	{
		numBlocks = numItems;
		// Calculate required alignment based on minimum device offset alignment
		uint64_t minUboAlignment = system.GetDeviceProperties().limits.minUniformBufferOffsetAlignment;
		blockSize = dataSize;
		if (minUboAlignment > 0 && (dataSize % minUboAlignment) > 0)
			blockSize += minUboAlignment - (dataSize % minUboAlignment);	// Align to device's alignment

		uniformBuffer.Create(system, GetDataSize(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, debugName);

		data = (char*)uniformBuffer.Map(system.GetDevice());
	}
	void Tidy(VulkanSystem& system) override
	{
		uniformBuffer.DestroyBuffer(system);
	}
	void CopyToDevice(const VulkanSystem& system)
	{
		VkMappedMemoryRange memoryRange{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
		memoryRange.memory = uniformBuffer.GetDeviceMemory();
		memoryRange.size = GetDataSize();

		uniformBuffer.CopyDataRanges(system.GetDevice(), 1, &memoryRange);
	}
	void CopyRangesToDevice(const VulkanSystem& system, uint32_t numRanges, VkMappedMemoryRange* memoryRanges)
	{
		uniformBuffer.CopyDataRanges(system.GetDevice(), numRanges, memoryRanges);
	}

	uint32_t GetNumDynamicBuffers() const { return numBlocks; }

	Buffer& GetBuffer() { return uniformBuffer; }
	VkDeviceSize GetDataSize() { return blockSize * numBlocks; }
	uint32_t GetBlockSize() const { return (uint32_t)blockSize; }

	UBO_DATA& GetData(uint32_t item) { return *(UBO_DATA*)(data + item * blockSize); }
	UBO_DATA& operator() (uint32_t item) { return GetData(item); }

private:
	char* data;
	Buffer uniformBuffer;
	uint64_t blockSize;
	uint32_t numBlocks;
};
