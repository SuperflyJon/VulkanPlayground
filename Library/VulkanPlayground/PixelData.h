#pragma once

namespace gli
{
	class texture;
}

struct PixelData
{
	PixelData(uint32_t width, uint32_t height, uint32_t depth) : pixels(nullptr), width(width), height(height), mipLevels(depth), size(0), texture(nullptr), isCube(false) {}
	PixelData() : pixels(nullptr), width(0), height(0), mipLevels(0), size(0), texture(nullptr), isCube(false) {}
	~PixelData() { TidyUp(); }
	void TidyUp();

	void Load(const std::string& filename);
	void LoadKTX(const std::string& filename);
	void LoadKTXCube(const std::string& filename);
	void LoadKTXArray(const std::string& filename);

	unsigned char* pixels;
	uint32_t width, height;
	uint32_t mipLevels;
	uint64_t size;
	gli::texture* texture;
	bool isCube;
	uint32_t GetNumLayers() const;
	uint32_t GetNumFaces() const;
	uint32_t GetCubeFaceLevelSize(uint32_t face, uint32_t level) const;
	uint32_t GetCubeFaceLevelWidth(uint32_t face, uint32_t level) const;
	uint32_t GetCubeFaceLevelHeight(uint32_t face, uint32_t level) const;
};
