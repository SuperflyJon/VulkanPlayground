
#include "stdafx.h"
#include "Descriptor.h"
#include "Image.h"
#include "System.h"

VkDescriptorPool Descriptor::CreateDescriptorPool(const VulkanSystem& system, const std::vector<Descriptor*>& descriptors, uint32_t extraDescriptors, const std::string& debugName)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	uint32_t maxSets = 0;
	uint32_t uniformBuffers = 0;
	uint32_t dynamicBuffers = 0;
	uint32_t combinedTextures = 0;
	uint32_t separateTextures = 0;
	uint32_t imageAttachments = 0;

	for (auto descriptorDef : descriptors)
	{
		maxSets++;
		auto descriptor = descriptorDef;
		uniformBuffers += (uint32_t)descriptor->uniformBuffers.size();
		if (descriptor->pDynamicUniformBuffer != nullptr)
			dynamicBuffers++;
		if (!descriptor->textures.empty())
		{
			if (descriptor->separateSampler)
				separateTextures += (uint32_t)descriptor->textures.size();
			else
				combinedTextures += (uint32_t)descriptor->textures.size();
		}
		imageAttachments += descriptor->NumAttachmentImageViews();
	}

	if (uniformBuffers > 0)
	{
		VkDescriptorPoolSize uniformPool;
		uniformPool.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformPool.descriptorCount = uniformBuffers;
		poolSizes.push_back(uniformPool);
	}
	if (dynamicBuffers > 0)
	{
		VkDescriptorPoolSize dynamicUniformPool;
		dynamicUniformPool.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		dynamicUniformPool.descriptorCount = dynamicBuffers;
		poolSizes.push_back(dynamicUniformPool);
	}
	if (separateTextures > 0)
	{
		VkDescriptorPoolSize samplerPool;
		samplerPool.type = VK_DESCRIPTOR_TYPE_SAMPLER;
		samplerPool.descriptorCount = 1;
		poolSizes.push_back(samplerPool);

		VkDescriptorPoolSize texturePool;
		texturePool.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		texturePool.descriptorCount = separateTextures;
		poolSizes.push_back(texturePool);
	}
	if (combinedTextures > 0)
	{
		VkDescriptorPoolSize texturePool;
		texturePool.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		texturePool.descriptorCount = combinedTextures;
		poolSizes.push_back(texturePool);
	}
	if (imageAttachments > 0)
	{
		VkDescriptorPoolSize attachmentPool;
		attachmentPool.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		attachmentPool.descriptorCount = imageAttachments;
		poolSizes.push_back(attachmentPool);
	}

	VkDescriptorPoolCreateInfo descriptorPoolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolInfo.poolSizeCount = (uint32_t)poolSizes.size();
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = maxSets + extraDescriptors;

	VkDescriptorPool descriptorPool;
	CHECK_VULKAN(vkCreateDescriptorPool(system.GetDevice(), &descriptorPoolInfo, nullptr, &descriptorPool), "Failed to create descriptor pool");
	system.DebugNameObject(descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "Descriptor Pool", debugName);
	return descriptorPool;
}

void Descriptor::CreateDescriptorSet(const VulkanSystem& system, VkDescriptorPool descriptorPool, uint32_t numDescriptors, const std::string& debugName)
{
	std::vector<VkDescriptorSetLayout> layouts(numDescriptors, descriptorSetLayout);
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptorSetAllocateInfo.descriptorPool = descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = numDescriptors;
	descriptorSetAllocateInfo.pSetLayouts = layouts.data();

	std::map<uint32_t, size_t> attahcmentImageCounts;

	descriptorSets.resize(numDescriptors);
	if (!CHECK_VULKAN(vkAllocateDescriptorSets(system.GetDevice(), &descriptorSetAllocateInfo, descriptorSets.data()), "Failed to allocate descriptor set"))
		return;

	for (auto descriptor : descriptorSets)
	{
		system.DebugNameObject(descriptor, VK_OBJECT_TYPE_DESCRIPTOR_SET, "Descriptor Set", debugName);

		std::vector<VkWriteDescriptorSet> descriptorWrites;

		uint32_t uniformBufferCount = 0;
		uint32_t attachmentCount = 0;
		uint32_t textureCount = 0;

		for (auto& binding : bindings)
		{
			VkWriteDescriptorSet writeDS{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			writeDS.dstSet = descriptor;
			writeDS.descriptorCount = 1;
			writeDS.dstBinding = binding.binding;
			writeDS.dstArrayElement = 0;
			writeDS.descriptorType = binding.descriptorType;

			switch (binding.descriptorType)
			{
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			{
				if (uniformBufferCount == uniformBuffers.size())
					throw std::runtime_error("Descriptor mismatch - no uniform buffer set");

				VkDescriptorBufferInfo * bufferInfo = new VkDescriptorBufferInfo();
				bufferInfo->buffer = uniformBuffers[uniformBufferCount]->GetBuffer();
				bufferInfo->range = uniformBuffers[uniformBufferCount]->GetBufferSize();

				writeDS.pBufferInfo = bufferInfo;
				uniformBufferCount++;
				break;
			}
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			{
				if (!pDynamicUniformBuffer)
					throw std::runtime_error("Descriptor mismatch - no dynamic uniform buffer set");

				VkDescriptorBufferInfo* bufferInfo = new VkDescriptorBufferInfo();
				bufferInfo->buffer = pDynamicUniformBuffer->GetBuffer();
				bufferInfo->range = pDynamicUniformBuffer->GetBufferSize() / numDynBuffs;

				writeDS.pBufferInfo = bufferInfo;
				break;
			}
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			{
				if (textures.empty())
					throw std::runtime_error("Descriptor mismatch - no texture set");

				uint32_t numTextures = binding.descriptorCount;

				VkDescriptorImageInfo* imageInfo = new VkDescriptorImageInfo[numTextures];
				for (uint32_t i = 0; i < numTextures; i++, textureCount++)
				{
					imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfo[i].imageView = textures[textureCount]->GetImageView();
					if (binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
						imageInfo[i].sampler = textures[textureCount]->GetSampler();
				}
				writeDS.descriptorCount = numTextures;
				writeDS.pImageInfo = imageInfo;
				break;
			}
			case VK_DESCRIPTOR_TYPE_SAMPLER:
			{
				VkDescriptorImageInfo* imageInfo = new VkDescriptorImageInfo[1];
				imageInfo->sampler = textures[0]->GetSampler();	// Only one sampler

				writeDS.pImageInfo = imageInfo;
				break;
			}
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
			{
				VkDescriptorImageInfo* imageInfo = new VkDescriptorImageInfo[1];
				imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				const ImageWithViewList& attachments = *attachmentImageViews[(uint32_t)attachmentCount];
				auto& index = attahcmentImageCounts[(uint32_t)attachmentCount];
				attachmentCount++;
				imageInfo->imageView = attachments[index]->GetImageView();
				if (index + 1 < attachments.size())
					index++;

				writeDS.pImageInfo = imageInfo;
				break;
			}
			default:
				throw std::runtime_error("Unknown descriptor type");
			}
			descriptorWrites.push_back(writeDS);
		}

		vkUpdateDescriptorSets(system.GetDevice(), (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

		for (auto& writeDS : descriptorWrites)
		{
			delete writeDS.pBufferInfo;
			delete [] writeDS.pImageInfo;
		}
	}
}

void Descriptor::Create(const VulkanSystem& system, VkDescriptorPool descriptorPool, const std::string& debugDescriptorName, const std::string& debugDescriptorLayoutName)
{
	// Create descriptor set layout
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptorSetLayoutInfo.bindingCount = (uint32_t)bindings.size();
	descriptorSetLayoutInfo.pBindings = bindings.data();

	CHECK_VULKAN(vkCreateDescriptorSetLayout(system.GetDevice(), &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout), "Failed to create descriptor set layout");
	system.DebugNameObject(descriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, "Descriptor Set Layout", (debugDescriptorLayoutName.empty() ? debugDescriptorName : debugDescriptorLayoutName));

	// Create the descriptor set
	CreateDescriptorSet(system, descriptorPool, 1/*numDescriptors*/, debugDescriptorName);	// Just use single descriptors?
}

void Descriptor::CreateShared(const VulkanSystem& system, VkDescriptorPool descriptorPool, VkDescriptorSetLayout otherDescriptorSetLayout, const std::string& debugName)
{
	sharedDescriptorSetLayout = true;
	descriptorSetLayout = otherDescriptorSetLayout;
	CreateDescriptorSet(system, descriptorPool, 1/*numDescriptors*/, debugName);	// Just use single descriptors?
}

void Descriptor::Tidy(VulkanSystem& system)
{
	for (auto buffer : uniformBuffers)
		buffer->DestroyBuffer(system);
	if (pDynamicUniformBuffer)
		pDynamicUniformBuffer->DestroyBuffer(system);
	for (auto& texture : textures)
		texture->Tidy(system);

	if (!sharedDescriptorSetLayout)
		vkDestroyDescriptorSetLayout(system.GetDevice(), descriptorSetLayout, nullptr);

	Reset();
}

void Descriptor::AddTextureSampler(uint32_t binding, VkShaderStageFlags stage)
{
	separateSampler = true;
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = binding;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = stage;
	bindings.push_back(samplerLayoutBinding);
}

void Descriptor::AddTexture(VulkanSystem& system, uint32_t binding, Texture& texture, const std::string& filename, VkFormat format, VkShaderStageFlags stage)
{
	texture.Load(system, filename, format);
	if (VulkanPlayground::showObjectCreationMessages)
		std::cout << "Loaded texture: " << filename << std::endl;

	AddTexture(binding, texture, stage);
}

void Descriptor::AddTexture(uint32_t binding, TextureBase& texture, VkShaderStageFlags stage)
{
	textures.push_back(&texture);
	AddTextureBinding(binding, 1, stage);
}

void Descriptor::AddTextureArray(uint32_t binding, Texture* textureArray, uint32_t numTextures, VkShaderStageFlags stage)
{
	for (uint32_t i = 0; i < numTextures; i++)
		textures.push_back(textureArray + i);

	AddTextureBinding(binding, numTextures, stage);
}

void Descriptor::AddTextureBinding(uint32_t binding, uint32_t numTextures, VkShaderStageFlags stage)
{
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = binding;
	if (!separateSampler)
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	else
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	samplerLayoutBinding.descriptorCount = numTextures;
	samplerLayoutBinding.stageFlags = stage;
	bindings.push_back(samplerLayoutBinding);
}

void Descriptor::AddSubPassInput(uint32_t binding, const ImageWithViewList& image, VkShaderStageFlags stage)
{
	attachmentImageViews[(uint32_t)attachmentImageViews.size()] = &image;

	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = stage;
	bindings.push_back(layoutBinding);
}
