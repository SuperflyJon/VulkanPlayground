
#include "stdafx.h"
#include "Image.h"
#include "PixelData.h"
#include "System.h"
#include "WinUtil.h"

#pragma warning(push)
#pragma warning(disable:26451 26495 6001 6262 6308 6387 28182 4310 4100)	// Disable some warnings in external code
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <gli/gli.hpp>
#pragma warning(pop)

void PixelData::Load(const std::string& filename)
{
	int w, h, channelsInImage;
	pixels = stbi_load(filename.c_str(), &w, &h, &channelsInImage, STBI_rgb_alpha);
	width = w;
	height = h;
	if (!pixels)
		throw std::runtime_error("Failed to load texture image");

	size = (uint64_t)width * (uint64_t)height * (uint64_t)4;	// 4 == rgba
	mipLevels = (uint32_t)std::floor(std::log2(std::max(width, height))) + 1;
}

void PixelData::LoadKTX(const std::string& filename)
{
	texture = new gli::texture2d(gli::load(filename.c_str()));

	gli::texture2d &text2d = *((gli::texture2d*)texture);
	width = text2d[0].extent().x;
	height = text2d[0].extent().y;
	mipLevels = (uint32_t)text2d.levels();
	size = text2d.size();
	pixels = (unsigned char*)text2d.data();
}

void PixelData::LoadKTXCube(const std::string& filename)
{
	isCube = true;
	texture = new gli::texture_cube(gli::load(filename.c_str()));

	width = texture->extent().x;
	height = texture->extent().y;
	mipLevels = (uint32_t)texture->levels();
	size = texture->size();

	pixels = (unsigned char*)texture->data(0, 0, 0);
}

void PixelData::LoadKTXArray(const std::string& filename)
{
	texture = new gli::texture2d_array(gli::load(filename.c_str()));

	width = texture->extent().x;
	height = texture->extent().y;
	mipLevels = (uint32_t)texture->levels();
	size = texture->size();

	pixels = (unsigned char*)texture->data(0, 0, 0);
}

uint32_t PixelData::GetNumLayers() const
{
	return (uint32_t)texture->layers();
}

uint32_t PixelData::GetNumFaces() const
{
	return (uint32_t)texture->faces();
}

uint32_t PixelData::GetCubeFaceLevelSize(uint32_t face, uint32_t level) const
{
	if (texture == nullptr)
		return height * width;

	if (face == 0)
	{
		return (uint32_t)texture->size(level);
	}
	else
	{
		if (isCube)
		{
			gli::texture_cube &textCube = *((gli::texture_cube*)texture);
			return (uint32_t)textCube[face][level].size();
		}
		else
		{
			gli::texture2d_array &textArray = *((gli::texture2d_array*)texture);
			return (uint32_t)textArray[face].size();
		}
	}
}

uint32_t PixelData::GetCubeFaceLevelWidth(uint32_t face, uint32_t level) const
{
	if (texture == nullptr)
		return width;

	if (face == 0)
	{
		return (uint32_t)texture->extent(level).x;
	}
	else
	{
		if (isCube)
		{
			gli::texture_cube &textCube = *((gli::texture_cube*)texture);
			return (uint32_t)textCube[face][level].extent().x;
		}
		else
		{
			gli::texture2d_array &textArray = *((gli::texture2d_array*)texture);
			return (uint32_t)textArray[face].extent().x;
		}
	}
}

uint32_t PixelData::GetCubeFaceLevelHeight(uint32_t face, uint32_t level) const
{
	if (texture == nullptr)
		return height;

	if (face == 0)
	{
		return (uint32_t)texture->extent(level).y;
	}
	else
	{
		if (isCube)
		{
			gli::texture_cube &textCube = *((gli::texture_cube*)texture);
			return (uint32_t)textCube[face][level].extent().y;
		}
		else
		{
			gli::texture2d_array &textArray = *((gli::texture2d_array*)texture);
			return (uint32_t)textArray[face].extent().y;
		}
	}
}

void PixelData::TidyUp()
{
	if (texture)
		delete texture;
	else if (pixels)
	{
		stbi_image_free(pixels);
		pixels = nullptr;
	}
}

void Texture::Load(VulkanSystem& system, const std::string& filename, VkFormat format, bool createSampler)
{
	if (!WinUtils::FileExists(filename))
		throw std::runtime_error("Texture file: " + filename + " does not exist");

	PixelData pd;
	bool isKTX = WinUtils::GetExtension(filename) == "ktx";
	if (isKTX)
		pd.LoadKTX(filename);
	else
		pd.Load(filename);

	system.CreateGpuImage(system, texture, pd, VK_IMAGE_USAGE_SAMPLED_BIT, format, (isKTX ? pd.GetNumFaces() : 0), WinUtils::GetJustFileName(filename));

	mipLevels = pd.mipLevels;
	textureImageView = VulkanPlayground::CreateImageView(system, texture.GetImage(), mipLevels, format, VK_IMAGE_ASPECT_COLOR_BIT, "Texture {" + WinUtils::GetJustFileName(filename) + "}");
	if (createSampler)
		CreateSampler(system, WinUtils::GetJustFileName(filename));
}

void Texture::LoadCubeMap(VulkanSystem& system, const std::string& filename, VkFormat format)
{
	PixelData pd;
	if (WinUtils::GetExtension(filename) == "ktx")
		pd.LoadKTXCube(filename);
	else
		throw std::runtime_error("Only ktx format supported for CubeMaps");

	mipLevels = pd.mipLevels;
	system.CreateGpuImage(system, texture, pd, VK_IMAGE_USAGE_SAMPLED_BIT, format, pd.GetNumFaces(), WinUtils::GetJustFileName(filename), VK_IMAGE_VIEW_TYPE_CUBE);
	textureImageView = VulkanPlayground::CreateImageView(system, texture.GetImage(), mipLevels, format, VK_IMAGE_ASPECT_COLOR_BIT, "CubeMap {" + filename + "}", VK_IMAGE_VIEW_TYPE_CUBE, 6);
	CreateSampler(system, WinUtils::GetJustFileName(filename));
}

void Texture::LoadArray(VulkanSystem& system, const std::string& filename, VkFormat format)
{
	PixelData pd;
	if (WinUtils::GetExtension(filename) == "ktx")
		pd.LoadKTXArray(filename);
	else
		throw std::runtime_error("Only ktx format supported for TextureArrays");

	numLayers = pd.GetNumLayers();
	mipLevels = pd.mipLevels;
	system.CreateGpuImage(system,texture, pd, VK_IMAGE_USAGE_SAMPLED_BIT, format, pd.GetNumFaces() * pd.GetNumLayers(), WinUtils::GetJustFileName(filename));
	textureImageView = VulkanPlayground::CreateImageView(system, texture.GetImage(), mipLevels, format, VK_IMAGE_ASPECT_COLOR_BIT, "TextureArray {" + filename + "}", VK_IMAGE_VIEW_TYPE_2D_ARRAY, pd.GetNumFaces() * pd.GetNumLayers());
	CreateSampler(system, WinUtils::GetJustFileName(filename));
}

void Texture::Create2dTexture(VulkanSystem& system, PixelData& pd, VkFormat format, const std::string& debugName)
{
	pd.mipLevels = mipLevels = 1;
	pd.size = pd.width * pd.height * Attribs::FormatSize(format);
	system.CreateGpuImage(system, texture, pd, VK_IMAGE_USAGE_SAMPLED_BIT, format, 1, debugName);
	auto inc = system.GetScopedDebugOutputIncrement();
	textureImageView = VulkanPlayground::CreateImageView(system, texture.GetImage(), mipLevels, format, VK_IMAGE_ASPECT_COLOR_BIT, debugName, VK_IMAGE_VIEW_TYPE_2D, 1);
	CreateSampler(system, debugName);
}

void Texture::Create3dTexture(VulkanSystem& system, PixelData& pd, VkFormat format, const std::string& debugName)
{
	uint32_t Layers3d = pd.mipLevels;
	pd.mipLevels = mipLevels = 1;
	system.CreateGpuImage(system, texture, pd, VK_IMAGE_USAGE_SAMPLED_BIT, format, Layers3d, debugName, VK_IMAGE_VIEW_TYPE_3D);
	textureImageView = VulkanPlayground::CreateImageView(system, texture.GetImage(), mipLevels, format, VK_IMAGE_ASPECT_COLOR_BIT, debugName, VK_IMAGE_VIEW_TYPE_3D, 1);
	CreateSampler(system, debugName);
}

void Texture::Load3dTexture(VulkanSystem& system, const std::string& filename, VkFormat format)
{
	PixelData pd;
	if (WinUtils::GetExtension(filename) == "ktx")
		pd.LoadKTXArray(filename);
	else
		throw std::runtime_error("Only ktx format supported for cubemaps");

	numLayers = pd.GetNumLayers();
	pd.mipLevels = pd.GetNumFaces() * pd.GetNumLayers();
	Create3dTexture(system, pd, format, WinUtils::GetJustFileName(filename));
}

void Texture::LoadCubeIntoTextures(VulkanSystem& system, Texture textures[6], const std::string& filename, VkFormat format)
{
	PixelData pd;
	if (WinUtils::GetExtension(filename) == "ktx")
		pd.LoadKTXCube(filename);
	else
		throw std::runtime_error("Only ktx format supported for cubemaps");

	pd.size = pd.size / 6;
	auto pixels = pd.pixels;
	for (int i = 0; i < 6; i++)
	{
		pd.pixels = pixels;
		pixels += pd.size;
		system.CreateGpuImage(system, textures[i].texture, pd, VK_IMAGE_USAGE_SAMPLED_BIT, format, 1, WinUtils::GetJustFileName(filename));
		std::stringstream ss;
		ss << "CubeTexture [" << i << "]";
		textures[i].textureImageView = VulkanPlayground::CreateImageView(system, textures[i].texture.GetImage(), pd.mipLevels, format, VK_IMAGE_ASPECT_COLOR_BIT, ss.str() + " {" + filename + "}");
		textures[1].textureSampler = nullptr;
	}

	textures[0].textureSampler = VulkanPlayground::CreateSampler(system, pd.mipLevels, textures[0].anisotopyLevel, textures[0].addressMode, textures[0].borderColor, textures[0].compareOp, WinUtils::GetJustFileName(filename));
}

void Texture::LoadTextureArrayIntoTextures(VulkanSystem& system, std::vector<Texture>& textures, const std::string& filename, VkFormat format)
{
	PixelData pd;
	if (WinUtils::GetExtension(filename) == "ktx")
		pd.LoadKTXArray(filename);
	else
		throw std::runtime_error("Only ktx format supported for cubemaps");

	textures.resize(pd.GetNumLayers());
	pd.size = pd.size / pd.GetNumLayers();
	auto pixels = pd.pixels;
	for (uint32_t i = 0; i < pd.GetNumLayers(); i++)
	{
		pd.pixels = pixels;
		pixels += pd.size;
		system.CreateGpuImage(system, textures[i].texture, pd, VK_IMAGE_USAGE_SAMPLED_BIT, format, 1, WinUtils::GetJustFileName(filename) + "{" + std::to_string(i) + "}");
		std::stringstream ss;
		ss << "TextureArray [" << i << "]";
		textures[i].textureImageView = VulkanPlayground::CreateImageView(system, textures[i].texture.GetImage(), pd.mipLevels, format, VK_IMAGE_ASPECT_COLOR_BIT, ss.str() + " {" + filename + "}", VK_IMAGE_VIEW_TYPE_2D);
		textures[i].textureSampler = nullptr;
	}
	textures[0].textureSampler = VulkanPlayground::CreateSampler(system, pd.mipLevels, textures[0].anisotopyLevel, textures[0].addressMode, textures[0].borderColor, textures[0].compareOp, WinUtils::GetJustFileName(filename));	// Single sampler
}

void Image::Create(VulkanSystem& system, VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height, uint32_t mipLevels, VkMemoryPropertyFlags properties, const std::string& debugName, std::set<uint32_t> queueIndicies, uint32_t numImages, VkImageViewType imageType)
{
	VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent = { width, height, 1 };
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = numImages;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	if (imageType == VK_IMAGE_VIEW_TYPE_CUBE)
	{
		if (numImages != 6)
			throw std::runtime_error("Cube images need 6 faces");
		imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;	// This flag is required for cube map images
	}
	else if (imageType == VK_IMAGE_VIEW_TYPE_3D)
	{
		imageInfo.arrayLayers = 1;
		imageInfo.extent.depth = numImages;
		imageInfo.imageType = VK_IMAGE_TYPE_3D;
	}

	std::vector<uint32_t> vecQueueData(queueIndicies.begin(), queueIndicies.end());
	imageInfo.sharingMode = (vecQueueData.size() > 1) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.queueFamilyIndexCount = (uint32_t)vecQueueData.size();
	imageInfo.pQueueFamilyIndices = vecQueueData.data();

	CHECK_VULKAN(vkCreateImage(system.GetDevice(), &imageInfo, nullptr, &image), "Failed to create image");
	system.DebugNameObject(image, VK_OBJECT_TYPE_IMAGE, "Image", debugName);
	auto inc = system.GetScopedDebugOutputIncrement();

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(system.GetDevice(), image, &memRequirements);

	VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memRequirements.size;
	allocateInfo.memoryTypeIndex = system.FindMemoryType(memRequirements.memoryTypeBits, properties);

	CHECK_VULKAN(vkAllocateMemory(system.GetDevice(), &allocateInfo, nullptr, &deviceMemory), "Failed to allocate buffer memory");
	CHECK_VULKAN(vkBindImageMemory(system.GetDevice(), image, deviceMemory, 0), "Failed to bind buffer");
	system.DebugNameObject(GetDeviceMemory(), VK_OBJECT_TYPE_DEVICE_MEMORY, "ImageMem ", debugName);

	created = true;
}

void Image::FreeBufferInternal(VulkanSystem& system)
{
	vkDestroyImage(system.GetDevice(), image, nullptr);
	image = nullptr;
}

void ImageWithView::Tidy(VulkanSystem& system)
{
	if (imageView != nullptr)
	{
		vkDestroyImageView(system.GetDevice(), imageView, nullptr);
		imageView = nullptr;
	}
	image.DestroyBuffer(system);
}

void TextureBase::Tidy(VulkanSystem& system)
{
	if (textureSampler != nullptr)
	{
		vkDestroySampler(system.GetDevice(), textureSampler, nullptr);
		textureSampler = nullptr;
	}
}

void Texture::Tidy(VulkanSystem& system)
{
	TextureBase::Tidy(system);
	if (textureImageView != nullptr)
	{
		vkDestroyImageView(system.GetDevice(), textureImageView, nullptr);
		textureImageView = nullptr;
	}
	texture.DestroyBuffer(system);
	Reset();
}
