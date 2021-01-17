#include "stdafx.h"
#include "Model.h"
#include "AssImp.h"
#include "Pipeline.h"
#include "WinUtil.h"
#include "System.h"
#include "EventData.h"
#include "Camera.h"

void Model::LoadToGpu(VulkanSystem& system, const std::string& modelFilename, const std::vector<Attribs::Attrib>& attribs)
{
	attribsUsed = attribs;

	if (vertices.empty())
		Load(modelFilename, attribs);

	if (!vertexBuffer.Created())
		CopyDataToGpu(system);
}

void Model::Load(const std::string& modelFilename, const std::vector<Attribs::Attrib>& attribs)
{
	vertexStride = Attribs::GetStride(attribs);

	auto tStart = std::chrono::high_resolution_clock::now();

	auto scene = AssImp::ImportFile(modelFilename, correctDodgyModel);
	if (scene == nullptr)
		throw std::runtime_error("Error loading model: " + AssImp::GetErrorString());

	AssImp::ProcessScene(*scene, *this, attribs, translation);

	AssImp::Tidy(scene);

	useIndices = !indices.empty();

	auto tEnd = std::chrono::high_resolution_clock::now();
	auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

	if (VulkanPlayground::showObjectCreationMessages)
	{
		std::cout << "Loaded model " << modelFilename << " in " << tDiff << "ms. ";
		if (useIndices)
		{
			auto bytesUsed = vertices.size() + indices.size() * sizeof(indices[0]);
			auto nonIndexedBytesUsed = indices.size() * vertexStride;
			std::cout << WinUtils::ThousandSep(vertices.size() / vertexStride) << " vertices, " << WinUtils::ThousandSep(indices.size()) << " indices, " << WinUtils::FormatDataSize(bytesUsed) << " (indices saved " << WinUtils::FormatDataSize(nonIndexedBytesUsed - bytesUsed) << ")\n";
		}
		else
		{
			std::cout << WinUtils::ThousandSep(vertices.size() / vertexStride) << " vertices, " << WinUtils::FormatDataSize(vertices.size()) << "\n";
		}
	}
}

void Model::CopyDataToGpu(VulkanSystem& system)
{
	system.CreateGpuBuffer(system, vertexBuffer, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Model Verticies");
	if (useIndices)
	{
		system.CreateGpuBuffer(system, indexBuffer, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, "Model Indicies");
	}
}

void Model::Draw(VkCommandBuffer commandBuffer, bool bindBuffers, uint32_t instanceCount)
{
	if (bindBuffers)
	{
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.GetBuffer(), VulkanPlayground::zeroOffset);
		if (useIndices)
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}
	if (useIndices)
		vkCmdDrawIndexed(commandBuffer, (uint32_t)indices.size(), instanceCount, 0, 0, 0);
	else
		vkCmdDraw(commandBuffer, GetNumVertices(), instanceCount, 0, 0);
}

void Model::CalculateExtents()
{
	if (!extentCalculated)
	{
		char* data = vertices.data();
		for (auto& attrib : attribsUsed)
		{
			if (attrib.type == Attribs::Type::Position)
				break;
			data += Attribs::FormatSize(attrib.format);
		}

		for (uint32_t vertexNum = 0; vertexNum < GetNumVertices(); vertexNum++)
		{
			float* vertexData = reinterpret_cast<float*>(data);
			for (int dim = 0; dim < 3; dim++)
			{
				if (vertexNum == 0 || vertexData[dim] < min[dim])
					min[dim] = vertexData[dim];
				if (vertexNum == 0 || vertexData[dim] > max[dim])
					max[dim] = vertexData[dim];
			}
			data += vertexStride;
		}
		extentCalculated = true;
	}
}

void Model::CalcPositionMatrix(glm::mat4& model, glm::vec3 modelRotation, CameraOrientator& camera, glm::vec3 offset, float yaw, float pitch)
{
	CalculateExtents();
	
	glm::vec3 modelSize((glm::abs(min) + glm::abs(max)));
	glm::vec3 originOffset = (modelSize * offset);
	camera.Reset(originOffset, yaw, pitch);

	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(modelRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(modelRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(modelRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
}

void Model::LookatCentered(MVP& mvp, float aspectRatio, glm::vec3 modelRotation, glm::vec3 viewYPO, glm::vec3 lookYP, float fov, float zNear, float zFar)
{
	CalculateExtents();

	glm::vec3 originOffset((min + max) * glm::vec3(0.5, 0.5, 0.5));

	mvp.model = glm::mat4(1.0f);
	mvp.model = glm::rotate(mvp.model, modelRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
	mvp.model = glm::rotate(mvp.model, modelRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
	mvp.model = glm::rotate(mvp.model, modelRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

	mvp.model = glm::translate(mvp.model, -originOffset);

	glm::mat4 viewPosMat(1.0f);
	viewPosMat = glm::rotate(viewPosMat, viewYPO.y, glm::vec3(0.0f, 1.0f, 0.0f));
	viewPosMat = glm::rotate(viewPosMat, viewYPO.x, glm::vec3(1.0f, 0.0f, 0.0f));
	viewPos = viewPosMat * glm::vec4(0, -max.z * viewYPO.z, 0, 1);
	mvp.view = VulkanPlayground::CalcViewMatrix(viewPos, lookYP.x, lookYP.y);
	mvp.projection = glm::perspective(glm::radians(fov), aspectRatio, zNear, zFar);
}
