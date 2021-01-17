#pragma once

#include "Common.h"
#include "Buffers.h"
#include "UBO.h"

class Texture;
class TextureBase;
class BufferManager;
class Image;
class ImageWithView;
typedef std::vector<ImageWithView*> ImageWithViewList;

class Descriptor : public ITidy
{
public:
	Descriptor()
	{
		Reset();
	}

	Descriptor(const Descriptor& other) = delete;

	void Reset()
	{
		descriptorSetLayout = nullptr;
		sharedDescriptorSetLayout = false;
		pDynamicUniformBuffer = nullptr;
		numDynBuffs = 0;
		separateSampler = false;
		uniformBuffers.clear();
		textures.clear();
		bindings.clear();
		attachmentImageViews.clear();
		descriptorSets.clear();
	}

	void Create(const VulkanSystem& system, VkDescriptorPool descriptorPool, const std::string& debugDescriptorName, const std::string& debugDescriptorLayoutName = "");
	void CreateShared(const VulkanSystem& system, VkDescriptorPool descriptorPool, VkDescriptorSetLayout otherDescriptorSetLayout, const std::string& debugName);

	void Tidy(VulkanSystem& system) override;

	template <class T>
	void AddUniformBuffer(VulkanSystem& system, uint32_t binding, UBO<T>& ubo, const std::string& debugName, VkShaderStageFlags stage = VK_SHADER_STAGE_VERTEX_BIT)
	{
		Buffer* buffer = &ubo.GetBuffer();
		uniformBuffers.push_back(buffer);

		if (!buffer->Created())
			ubo.Create(system, "UBO [" + debugName + "]");

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = binding;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = stage;
		bindings.push_back(uboLayoutBinding);
	}
	template <class T>
	void AddDynamicUniformBuffer(VulkanSystem& system, uint32_t binding, DynamicUBO<T>& ubo, uint32_t numBuffs, const std::string& debugName, VkShaderStageFlags stage = VK_SHADER_STAGE_VERTEX_BIT)
	{
		numDynBuffs = numBuffs;
		pDynamicUniformBuffer = &ubo.GetBuffer();

		if (!pDynamicUniformBuffer->Created())
			ubo.Create(system, sizeof(T), numBuffs, "DynUBO [" + debugName + "]");

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = binding;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = stage;
		bindings.push_back(uboLayoutBinding);
	}
	void AddTexture(uint32_t binding, TextureBase& texture, VkShaderStageFlags stage = VK_SHADER_STAGE_FRAGMENT_BIT);
	void AddTexture(VulkanSystem& system, uint32_t binding, Texture& texture, const std::string& filename, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, VkShaderStageFlags stage = VK_SHADER_STAGE_FRAGMENT_BIT);
	void AddTextureArray(uint32_t binding, Texture* textureArray, uint32_t numTextures, VkShaderStageFlags stage = VK_SHADER_STAGE_FRAGMENT_BIT);
	void AddTextureSampler(uint32_t binding, VkShaderStageFlags stage = VK_SHADER_STAGE_FRAGMENT_BIT);
	void AddSubPassInput(uint32_t binding, const ImageWithViewList& image, VkShaderStageFlags stage = VK_SHADER_STAGE_FRAGMENT_BIT);
	uint32_t NumAttachmentImageViews() const { return (uint32_t)attachmentImageViews.size(); }

	VkDescriptorSetLayout& GetDescriptorSetLayout() { return descriptorSetLayout; }
	const VkDescriptorSet& GetDescriptorSet(uint32_t num = 0) const { return descriptorSets[num]; }

	static VkDescriptorPool CreateDescriptorPool(const VulkanSystem& system, const std::vector<Descriptor*>& descriptors, uint32_t numDescriptors, const std::string& debugName);

protected:
	void AddTextureBinding(uint32_t binding, uint32_t numTextures, VkShaderStageFlags stage);

	void CreateDescriptorSet(const VulkanSystem& system, VkDescriptorPool descriptorPool, uint32_t numDescriptors, const std::string& debugName);

private:
	Buffer* pDynamicUniformBuffer;
	uint32_t numDynBuffs;
	std::vector<Buffer*> uniformBuffers;
	std::map<uint32_t, const ImageWithViewList*> attachmentImageViews;
	std::vector<TextureBase*> textures;
	bool separateSampler;

	VkDescriptorSetLayout descriptorSetLayout;
	bool sharedDescriptorSetLayout;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
};
