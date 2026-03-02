#include <VulkanPlayground\Includes.h>

class TriangleSimpleApp : public VulkanApplication
{
	void SetupObjects(VulkanSystem& system, RenderPass& renderPass, VkExtent2D workingExtent) override;
	void DrawScene(VkCommandBuffer commandBuffer) override;

	Pipeline pipeline;
	Buffer vertexBuffer;
};
