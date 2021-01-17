#pragma once

#include "Camera.h"
#include "UBO.h"

#if defined(COMBINED_EXAMPLES)
#define DECLARE_APP(name) \
	VulkanApplication& Create ## name ## App() \
	{ \
		auto& app = *new name ## App; \
		return app; \
	}
#else
#define DECLARE_APP(name) \
int main() \
{ \
	name ## App app; \
	WindowSystem::RunWindowed(600, 500, #name, app); \
}
#endif

class SwapChain;
class RenderPass;
class RenderPasses;
class EventData;
class VulkanSystem;
class Pipeline;
class GLFW;
class Model;
class TypeSetter;
struct MouseData;
class FPSTimer;
class ITextHelper;

class VulkanApplication : public ITidy
{
public:
	VulkanApplication();
	~VulkanApplication();
	void Starting(const VulkanSystem& system);

	// Setup all graphics objects
	virtual void SetupObjects(VulkanSystem& /*system*/, RenderPass& /*renderPass*/, VkExtent2D /*workingExtent*/) { }
	virtual void SetupObjects(VulkanSystem& system, RenderPasses& renderPasses, VkExtent2D workingExtent);
	// Setup Camera and matrices etc
	virtual void ResetScene() { }
	// Update Camera
	virtual void UpdateCamera(float /*frameTime*/, EventData& /*eventData*/) { }
	// Update frame related data
	virtual void UpdateScene(VulkanSystem& /*system*/, float /*frameTime*/) { }
	// Draw the scene
	virtual void DrawScene(VkCommandBuffer /*commandBuffer*/) { }
	// Handle keyboard presses
	virtual void ProcessKeyPresses(const EventData& /*eventData*/) { }
	// Handle mouse movement
	virtual void ProcessMouseMovement(const MouseData& /*mouseData*/) { }
	// Override for custom renderpass creation
	virtual void SetupRenderPasses(RenderPasses& renderPasses, VulkanSystem& system, VkFormat imageFormat);
	// Specifiy any optional features that are required
	virtual void GetRequiredDeviceFeatures(const VkPhysicalDeviceFeatures& /*deviceFeatures*/, VkPhysicalDeviceFeatures* /*requiredFeatures*/) {}
	// Override to control depth/stencil buffer usage
	virtual bool UseDepthBuffer() { return true; }
	virtual bool UseStencilBuffer() { return false; }

	virtual glm::vec4 GetClearColour() const { return clearColour; }

	// Application event handling
	virtual void AppProcessKeyPresses(const EventData& eventData);
	virtual void AppProcessMouseMovement(const MouseData& mouseData);

	void CreatePipeline(VulkanSystem& system, const RenderPass& renderPass, Pipeline& pipeline, const VkExtent2D& viewExtent, const std::string& debugName, VkDescriptorSetLayout descriptorLayout = nullptr);
	void CreatePipeline(VulkanSystem& system, const RenderPass& renderPass, Pipeline& pipeline, Descriptor& descriptor, const VkExtent2D& viewExtent, const std::string& debugName);
	void CreateDescriptors(VulkanSystem& system, const std::vector<Descriptor*>& descriptorsInput, const std::string& debugDescriptorName, uint32_t extraDescriptors = 0);
	void CreateDescriptor(VulkanSystem& system, Descriptor& descriptor, const std::string& debugDescriptorName);
	void CreateBaseDescriptor(VulkanSystem& system, Descriptor& descriptor, const std::string& debugDescriptorName, uint32_t numDescriptors);
	void CreateSharedDescriptor(const VulkanSystem& system, Descriptor& descriptor, VkDescriptorSetLayout otherDescriptorSetLayout, const std::string& debugName);

	unsigned int GetWindowWidth() const { return windowWidth; }
	unsigned int GetWindowHeight() const { return windowHeight; }
	float AspectRatio() const { return (float)windowWidth / (float)windowHeight; }

	void TidyObjectOnExit(ITidy& object) { tidyObjects.push_back(&object); }

	bool ObjectsCreated(VulkanSystem& system, RenderPasses& renderPasses);
	void RecreateObjects() { objectsCreated = false; }
	void RedrawScene() { redrawScene = true; }
	bool RedrawSceneSet() {
		bool ret = redrawScene;
		redrawScene = false;
		return ret;
	}

	void SetTextHelper(ITextHelper* helper);
	ITextHelper* GetTextHelper();

	void CheckResetScene();

	void GetWindowSize(GLFW& window);

	virtual void CreateCommandBuffers(VulkanSystem& system, RenderPasses& renderPasses);

	void SetupWindowObjects(SwapChain& swapChain, VkSurfaceKHR windowSurface, VulkanSystem& system, RenderPasses& renderPasses);
	void Tidy(VulkanSystem& system) override;
	void TidyDescriptors(VulkanSystem& system);
	void TidyPipelines(VulkanSystem& system);

	TypeSetter& PrintString(TypeSetter& typeSetter, const UnicodeString& unicodeString, glm::vec3 fontColour = glm::vec3(1.0f, 1.0f, 1.0f), float scale = 1.0f);
	TypeSetter& PrintString(Vulkan2DFont& font, int x, int y, const UnicodeString& unicodeString, glm::vec3 fontColour = glm::vec3(1.0f, 1.0f, 1.0f), float scale = 1.0f, unsigned int width = 1000, unsigned int height = 1000);
	TypeSetter& PrintString(int x, int y, const UnicodeString& unicodeString, glm::vec3 fontColour = glm::vec3(1.0f, 1.0f, 1.0f), float scale = 1.0f, unsigned int width = 1000, unsigned int height = 1000);

	void ResetPrints(bool clear);
	void ProcessPrints(VulkanSystem& system, VkExtent2D workingExtent);

	virtual void AppSetupObjects(VulkanSystem& system, RenderPasses& renderPasses, VkExtent2D workingExtent);
	virtual void AppResetScene() {}
	virtual void AppUpdateScene(VulkanSystem& system, const FPSTimer& frameTime);

	void SetClearColour(const glm::vec4& val) { clearColour = val; }

	VkFormat GetDepthFormat(VulkanSystem& system);

	void SetTitle(const UnicodeString& name) { title = name; }
	const UnicodeString& GetTitle() const { return title; }

protected:
	int windowWidth, windowHeight;
	bool objectsCreated;
	bool redrawScene;
	bool resetScene;
	bool showFPS;
	bool vSync;
	std::vector<Pipeline*> pipelines;
	std::vector<Descriptor*> descriptors;
	std::vector<VkDescriptorPool> descriptorPools;
	std::vector<ITidy*> tidyObjects;

	glm::vec4 clearColour;
	VkFormat depthBufferFormat;

	RenderPass* fontRenderPass;
	std::unique_ptr<ITextHelper> textHelper;
	UnicodeString title;
};

class VulkanApplication3D : virtual public VulkanApplication
{
public:
	void UpdateCamera(float frameTime, EventData& eventData) override;

	void AppSetupObjects(VulkanSystem& system, RenderPasses& renderPasses, VkExtent2D workingExtent) override;
	void AppResetScene() override;
	void AppUpdateScene(VulkanSystem& system, const FPSTimer& frameTime) override;

	void CalcPositionMatrix(glm::vec3 modelRotation, glm::vec3 offset, float yaw, float pitch, const glm::vec3& modelSize = glm::vec3(1.0f));
	void CalcPositionMatrixMoveBack(float factor, const glm::vec3& modelSize = glm::vec3(1.0f));
	void CenterModel(Model& model);

	CameraOrientator& GetCameraPos() { return cameraPos; }
	const CameraOrientator& GetCameraPos() const { return cameraPos; }

	CameraOrientator& GetWorldPos() { return worldPos; }
	const CameraOrientator& GetWorldPos() const { return worldPos; }

	void AppProcessMouseMovement(const MouseData& mouseData) override;

protected:
	CameraOrientator cameraPos, worldPos;
	UBO<MVP> mvpUBO;
};

class VulkanApplication3DLightBase : virtual public VulkanApplication3D
{
public:
	VulkanApplication3DLightBase() : useEyeLightSpace(false)
	{ }
	void UseEyeLightSpace() { useEyeLightSpace = true; }

protected:
	bool useEyeLightSpace;
};

class VulkanApplication3DSimpleLight : virtual public VulkanApplication3DLightBase
{
public:
	void AppUpdateScene(VulkanSystem& system, const FPSTimer& frameTime) override;
	void SetupLighting(glm::vec3 offset, std::array<float, 4> cutoff, std::array<float, 4> value, float brightFactor, const glm::vec3& modelSize = glm::vec3(1.0f)) { SetupLighting(lightToonUBO, offset, cutoff, value, brightFactor, modelSize); }
	static void SetupLighting(UBO<UBO_lightToon>& lightToonUBO, glm::vec3 offset, std::array<float, 4> cutoff, std::array<float, 4> value, float brightFactor, const glm::vec3& modelSize = glm::vec3(1.0f));

protected:
	UBO<UBO_lightToon> lightToonUBO;
};

class VulkanApplication3DLight : virtual public VulkanApplication3DLightBase
{
public:
	VulkanApplication3DLight() : ambient(0), diffuse(0), specular(0), shineness(0), showAmbient(true), showDiffuse(true), showSpecular(true)
	{}

	void AppSetupObjects(VulkanSystem& system, RenderPasses& renderPasses, VkExtent2D workingExtent) override;
	void AppProcessKeyPresses(const EventData& eventData) override;
	void AppUpdateScene(VulkanSystem& system, const FPSTimer& frameTime) override;
	void AppResetScene() override;

	void SetupLighting(glm::vec3 offset, float ambientLevel, float diffuseLevel, float specularLevel, float shinenessLevel, const glm::vec3& modelSize = glm::vec3(1.0f));
	void SetLightLevels(float ambientLevel, float diffuseLevel, float specularLevel, float shinenessLevel);
	void UpdateLevels();

protected:
	float ambient, diffuse, specular, shineness;
	bool showAmbient, showDiffuse, showSpecular;
	UBO<UBO_light> lightUBO;
};
