#pragma once

#include "Buffers.h"

class Image : public BufferBase
{
public:
	/*explicit*/ Image() : image(nullptr)
	{}
	void Create(VulkanSystem& system, VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height, uint32_t mipLevels, VkMemoryPropertyFlags properties, const std::string& debugName, std::set<uint32_t> queueIndicies = {}, uint32_t numImages = 1, VkImageViewType imageType = VK_IMAGE_VIEW_TYPE_2D);

	VkImage GetImage() const { return image; }

protected:
	void FreeBufferInternal(VulkanSystem& system) override;

private:
	VkImage image;
};

class ImageWithView : public ITidy
{
public:
	ImageWithView() : imageView(nullptr)
	{
	}

	void Create(VulkanSystem& system, VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height, uint32_t mipLevels, VkMemoryPropertyFlags properties, VkImageAspectFlagBits imageAspectFlag, const std::string& debugName)
	{
		image.Create(system, format, usage, width, height, mipLevels, properties, debugName);
		imageView = VulkanPlayground::CreateImageView(system, image.GetImage(), 1, format, imageAspectFlag, debugName);
	}
	bool Created() const { return image.Created(); }
	VkImage GetImage() const { return image.GetImage(); }
	VkImageView GetImageView() const { return imageView; }

	void Tidy(VulkanSystem& system) override;
	
private:
	Image image;
	VkImageView imageView;
};

class TextureBase : public ITidy
{
public:
	TextureBase()
	{
		Reset();	// Duplicated here to shutup VS intelliSense helpers
	}
	void Reset()
	{
		mipLevels = 0;
		numLayers = 0;
		anisotopyLevel = 0;
		textureSampler = nullptr;
		textureImageView = nullptr;
		addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		compareOp = VK_COMPARE_OP_ALWAYS;
	}
	uint32_t GetAnisotopyLevel() const { return anisotopyLevel; }
	void SetAnisotopyLevel(uint32_t level) { anisotopyLevel = level; }
	void SetSamplerAddressMode(VkSamplerAddressMode mode) { addressMode = mode; }
	void SetBorderColor(VkBorderColor colour) { borderColor = colour; }
	void SetCompareOp(VkCompareOp op) { compareOp = op; }

	VkImageView GetImageView() const { return textureImageView; }
	VkSampler GetSampler() const { return textureSampler; }

	uint32_t GetNumLayers() const { return numLayers; }

	void SetImageView(VkImageView imageView) { textureImageView = imageView; }
	void CreateSampler(const VulkanSystem& system, const std::string& debugName) { textureSampler = VulkanPlayground::CreateSampler(system, mipLevels, anisotopyLevel, addressMode, borderColor, compareOp, debugName); }

	virtual void Tidy(VulkanSystem& system) override;

protected:
	VkImageView textureImageView;
	VkSampler textureSampler;

	uint32_t anisotopyLevel;	// Doesn't seem to make any difference in examples - needs a very fine view angle?
	unsigned int mipLevels;
	uint32_t numLayers;

	VkSamplerAddressMode addressMode;
	VkBorderColor borderColor;
	VkCompareOp compareOp;
};

class Texture : public TextureBase
{
public:
	void Load(VulkanSystem& system, const std::string& filename, VkFormat format, bool createSampler = true);
	void LoadCubeMap(VulkanSystem& system, const std::string& filename, VkFormat format);
	void LoadArray(VulkanSystem& system, const std::string& filename, VkFormat format);
	void Load3dTexture(VulkanSystem& system, const std::string& filename, VkFormat format);
	void Create2dTexture(VulkanSystem& system, PixelData& pd, VkFormat format, const std::string& debugName);
	void Create3dTexture(VulkanSystem& system, PixelData& pd, VkFormat format, const std::string& debugName);

	static void LoadCubeIntoTextures(VulkanSystem& system, Texture textures[6], const std::string& filename, VkFormat format);
	static void LoadTextureArrayIntoTextures(VulkanSystem& system, std::vector<Texture>& textures, const std::string& filename, VkFormat format);

	void Tidy(VulkanSystem& system) override;

	unsigned int GetMipLevels() const { return mipLevels; }

	void SetAddressMode(VkSamplerAddressMode mode) { addressMode = mode; }

private:
	Image texture;
};
