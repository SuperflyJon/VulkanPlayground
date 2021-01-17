#pragma once

#include "Common.h"
#include "Buffers.h"

class EventData;
class CameraOrientator;

class Model
{
public:
	Model() : translation(), useIndices(true), vertexStride(0), extentCalculated(false), correctDodgyModel(false)
	{}
	void DontUseIndicies() { useIndices = false; }

	void SetTranslation(const std::array<double, 3>& value) { translation = value; }

	void LoadToGpu(VulkanSystem& system, const std::string& modelFilename, const std::vector<Attribs::Attrib>& attribs);
	bool Loaded() const { return !vertices.empty(); }

	void Draw(VkCommandBuffer commandBuffer, bool bindBuffers = true, uint32_t instanceCount = 1);

	void CalcPositionMatrix(glm::mat4& model, glm::vec3 modelRotation, CameraOrientator& eventData, glm::vec3 offset, float yaw, float pitch);
	void LookatCentered(MVP& mvp, float aspectRatio, glm::vec3 modelRotation, glm::vec3 viewYPO, glm::vec3 lookYP, float fov = 45.0f, float zNear = 0.1f, float zFar = 100.0f);
	glm::vec3 GetModelSize() { CalculateExtents(); return (glm::abs(min) + glm::abs(max)); }
	glm::vec3 GetMinExtent() { CalculateExtents(); return min; }
	glm::vec3 GetMaxExtent() { CalculateExtents(); return max; }

	uint32_t GetNumVertices() const { return (uint32_t)vertices.size() / vertexStride; }

	void CorrectDodgyModelOnLoad() { correctDodgyModel = true; }

protected:
	void Load(const std::string& modelFilename, const std::vector<Attribs::Attrib>& attribs);
	void CopyDataToGpu(VulkanSystem& system);
	void CalculateExtents();

public:
	std::array<double, 3> translation;
	std::vector<char> vertices;
	std::vector<uint32_t> indices;
	Buffer vertexBuffer;
	Buffer indexBuffer;

	bool useIndices;
	uint32_t vertexStride;

	std::vector<Attribs::Attrib> attribsUsed;
	glm::vec3 min, max;
	bool extentCalculated;
	glm::vec4 viewPos;

	bool correctDodgyModel;

	friend class SceneProcessor;
};
