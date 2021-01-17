
#include "stdafx.h"
#include "Application.h"
#include "System.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "GLFW.h"
#include "EventData.h"
#include "WinUtil.h"
#include "Descriptor.h"
#include "BitmapFont.h"
#include "Model.h"
#include "FrameTimer.h"
#include "TextHelper.h"

VulkanApplication::VulkanApplication()
	: windowWidth(0), windowHeight(0), objectsCreated(false), redrawScene(false), resetScene(true), showFPS(false), vSync(true),
	  depthBufferFormat(VK_FORMAT_UNDEFINED), fontRenderPass(nullptr), textHelper(nullptr)
{
}
VulkanApplication::~VulkanApplication()
{
}

void VulkanApplication::Starting(const VulkanSystem& system)
{
	if (!system.fontsEnabled)
		textHelper.reset(new ITextHelper());	// Disable text functions
	else
		GetTextHelper()->LoadStrings();
}

void VulkanApplication::SetupObjects(VulkanSystem& system, RenderPasses& renderPasses, VkExtent2D workingExtent)
{
	SetupObjects(system, renderPasses.GetScreenRenderPass(), workingExtent);	// In general just using the main renderpass
}

void VulkanApplication::SetupWindowObjects(SwapChain& swapChain, VkSurfaceKHR windowSurface, VulkanSystem& system, RenderPasses& renderPasses)
{
	// For simplicity, recreate everything
	swapChain.Tidy(system);
	TidyPipelines(system);
	TidyDescriptors(system);
	renderPasses.Tidy(system);
	depthBufferFormat = VK_FORMAT_UNDEFINED;

	// Create new things
	swapChain.Create(system, windowSurface, windowWidth, windowHeight, vSync, "Application");
	SetupRenderPasses(renderPasses, system, swapChain.GetImageFormat());
	if (system.fontsEnabled)
	{	// Use separate render pass as main one might not be compatible
		fontRenderPass = &renderPasses.NewRenderPass();
		fontRenderPass->AddColourAttachment(swapChain.GetImageFormat(), "Fonts", VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		ResetPrints(true);
	}

	swapChain.CreateFrameBuffers(system, renderPasses, GetDepthFormat(system), "Application");

	AppSetupObjects(system, renderPasses, swapChain.GetWorkingExtent());

	RedrawScene();
	objectsCreated = true;
}

void VulkanApplication::AppSetupObjects(VulkanSystem& system, RenderPasses& renderPasses, VkExtent2D workingExtent)
{
	GetTextHelper()->SetupText(workingExtent);
	SetupObjects(system, renderPasses, workingExtent);
}

void VulkanApplication3D::AppSetupObjects(VulkanSystem& system, RenderPasses& renderPasses, VkExtent2D workingExtent)
{	
// Uncomment to show debug camera details
//#define SHOW_CAMERA_DETAILS

#ifdef SHOW_CAMERA_DETAILS
	// Preload string buffers + characters for debug prints (to avoid recreating fonts)
	AddPrintString("WldCamPosYaw:().e-0123456789");
#endif

	VulkanPlayground::SetupProjectionMatrix(mvpUBO().projection, AspectRatio());
	VulkanApplication::AppSetupObjects(system, renderPasses, workingExtent);
}

void VulkanApplication::CreateCommandBuffers(VulkanSystem& system, RenderPasses& renderPasses)
{
	std::vector<std::function<void(VkCommandBuffer)>> drawCmds;

	std::cout << "Draw\n";

	renderPasses.CreateCmdBuffers(0, system,
		[this, &system](VkCommandBuffer commandBuffer)
		{
			system.DebugStartRegion(commandBuffer, "Draw Scene", { 1.0f, 1.0f, 0.0f });
			DrawScene(commandBuffer);
			system.DebugEndRegion(commandBuffer);
		}, "DrawCmds");

	if (system.fontsEnabled)
		GetTextHelper()->CreateCommandBuffers(system, renderPasses);
}

void VulkanApplication::TidyDescriptors(VulkanSystem& system)
{
	for (auto descriptor : descriptors)
		descriptor->Tidy(system);
	descriptors.clear();

	for (auto pool : descriptorPools)
	{
		vkDestroyDescriptorPool(system.GetDevice(), pool, nullptr);
	}
	descriptorPools.clear();
}

void VulkanApplication::TidyPipelines(VulkanSystem& system)
{
	for (auto pipeline : pipelines)
		pipeline->Tidy(system);
	pipelines.clear();
}

void VulkanApplication::Tidy(VulkanSystem& system)
{
	TidyDescriptors(system);

	TidyPipelines(system);

	for (ITidy* object : tidyObjects)
		object->Tidy(system);

	GetTextHelper()->Tidy(system);

	RecreateObjects();
}

void VulkanApplication::CreateDescriptors(VulkanSystem& system, const std::vector<Descriptor*>& descriptorsInput, const std::string& debugDescriptorName, uint32_t extraDescriptors)
{
	auto descriptorPool = Descriptor::CreateDescriptorPool(system, descriptorsInput, extraDescriptors, debugDescriptorName);
	descriptorPools.push_back(descriptorPool);
	auto inc = system.GetScopedDebugOutputIncrement();

	for (auto descriptorDef : descriptorsInput)
	{
		descriptorDef->Create(system, descriptorPool, debugDescriptorName);
		descriptors.push_back(descriptorDef);
	}
}

void VulkanApplication::CreateDescriptor(VulkanSystem& system, Descriptor& descriptor, const std::string& debugDescriptorName)
{
	CreateDescriptors(system, { &descriptor }, debugDescriptorName);
}

void VulkanApplication::CreateBaseDescriptor(VulkanSystem& system, Descriptor& descriptor, const std::string& debugDescriptorName, uint32_t numDescriptors)
{
	CreateDescriptors(system, { &descriptor }, debugDescriptorName, numDescriptors - 1);
}

void VulkanApplication::CreateSharedDescriptor(const VulkanSystem& system, Descriptor& descriptor, VkDescriptorSetLayout otherDescriptorSetLayout, const std::string& debugName)
{
	descriptor.CreateShared(system, descriptorPools.back(), otherDescriptorSetLayout, debugName);
	descriptors.push_back(&descriptor);
}

void VulkanApplication::CreatePipeline(VulkanSystem& system, const RenderPass& renderPass, Pipeline& pipeline, Descriptor& descriptor, const VkExtent2D& viewExtent, const std::string& debugName)
{
	return CreatePipeline(system, renderPass, pipeline, viewExtent, debugName, descriptor.GetDescriptorSetLayout());
}

void VulkanApplication::CreatePipeline(VulkanSystem& system, const RenderPass& renderPass, Pipeline& pipeline, const VkExtent2D& viewExtent, const std::string& debugName, VkDescriptorSetLayout descriptorLayout)
{
	if (!pipeline.IsDynamicStateEnabled(VK_DYNAMIC_STATE_VIEWPORT))
	{
		VkRect2D viewport{};
		viewport.extent = viewExtent;
		pipeline.SetViewPort(viewport);
	}
	pipeline.Create(system, descriptorLayout, renderPass, debugName);
	pipelines.push_back(&pipeline);
}

void VulkanApplication::SetupRenderPasses(RenderPasses& renderPasses, VulkanSystem& system, VkFormat imageFormat)
{
	RenderPass& renderPass = renderPasses.NewRenderPass();
	renderPass.SetClearColour(GetClearColour());
	renderPass.AddColourAttachment(imageFormat, "MainBuffer");
	if (UseDepthBuffer())
		renderPass.AddDepthAttachment(GetDepthFormat(system));
}

VkFormat VulkanApplication::GetDepthFormat(VulkanSystem& system)
{
	if ((UseDepthBuffer() || UseStencilBuffer()) && depthBufferFormat == VK_FORMAT_UNDEFINED)
	{
		if (!UseStencilBuffer())
			depthBufferFormat = system.FindSupportedFormat({ VK_FORMAT_D32_SFLOAT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		if (depthBufferFormat == VK_FORMAT_UNDEFINED)
			depthBufferFormat = system.FindSupportedFormat({ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		if (depthBufferFormat == VK_FORMAT_UNDEFINED)
			throw std::runtime_error("Unable to find suitable depthbuffer format!");
	}
	return depthBufferFormat;
}

void VulkanApplication::GetWindowSize(GLFW& window)
{
	window.GetWindowSize(&windowWidth, &windowHeight);
}

void VulkanApplication3DLight::AppSetupObjects(VulkanSystem& system, RenderPasses& renderPasses, VkExtent2D workingExtent)
{
	VulkanApplication3D::AppSetupObjects(system, renderPasses, workingExtent);
}

void VulkanApplication3DLight::AppProcessKeyPresses(const EventData& eventData)
{
	if (eventData.KeyPressed('1'))
	{
		showAmbient = !showAmbient;
		UpdateLevels();
	}
	if (eventData.KeyPressed('2'))
	{
		showDiffuse = !showDiffuse;
		UpdateLevels();
	}
	if (eventData.KeyPressed('3'))
	{
		showSpecular = !showSpecular;
		UpdateLevels();
	}
	VulkanApplication3D::AppProcessKeyPresses(eventData);
}

void VulkanApplication3D::AppUpdateScene(VulkanSystem& system, const FPSTimer& frameTime)
{
	auto savedModel = mvpUBO().model;
	mvpUBO().model *= GetWorldPos().CalcViewMatrix();
	mvpUBO().view = GetCameraPos().CalcViewMatrix();
	VulkanApplication::AppUpdateScene(system, frameTime);
	if (mvpUBO.GetBuffer().Created())
		mvpUBO.CopyToDevice(system);
	mvpUBO().model = savedModel;

#ifdef SHOW_CAMERA_DETAILS
	auto &ts = PrintString(5, showFPS ? 90 : 70, "Cam: " + GetCameraPos().GetDebugString());
	PrintString(ts, "\nWld: " + GetWorldPos().GetDebugString());
	ts.AddExtraBuffer(50);
#endif
}

void VulkanApplication3DSimpleLight::AppUpdateScene(VulkanSystem& system, const FPSTimer& frameTime)
{
	auto savedLightPos = lightToonUBO().lightPos;
	VulkanApplication3DLightBase::AppUpdateScene(system, frameTime);

	if (useEyeLightSpace)
		lightToonUBO().lightPos = glm::vec3(mvpUBO().view * glm::vec4(lightToonUBO().lightPos, 1.0f));	// Convert lightPos to eye space

	if (lightToonUBO.GetBuffer().Created())
		lightToonUBO.CopyToDevice(system);
	lightToonUBO().lightPos = savedLightPos;
}

void VulkanApplication3DLight::AppUpdateScene(VulkanSystem& system, const FPSTimer& frameTime)
{
	lightUBO().viewPos = GetCameraPos().GetPosition();
	auto savedLightPos = lightUBO().lightPos;
	VulkanApplication3DLightBase::AppUpdateScene(system, frameTime);

	if (useEyeLightSpace)
		lightUBO().lightPos = glm::vec3(mvpUBO().view * glm::vec4(lightUBO().lightPos, 1.0f));	// Convert lightPos to eye space

	if (lightUBO.GetBuffer().Created())
		lightUBO.CopyToDevice(system);
	lightUBO().lightPos = savedLightPos;
}

void VulkanApplication3D::CalcPositionMatrix(glm::vec3 modelRotation, glm::vec3 offset, float yaw, float pitch, const glm::vec3& modelSize)
{
	glm::vec3 originOffset = (modelSize * offset);
	cameraPos.Reset(originOffset, yaw, pitch);

	mvpUBO().model = glm::mat4(1.0f);
	mvpUBO().model = glm::rotate(mvpUBO().model, glm::radians(modelRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	mvpUBO().model = glm::rotate(mvpUBO().model, glm::radians(modelRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	mvpUBO().model = glm::rotate(mvpUBO().model, glm::radians(modelRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
}

void VulkanApplication3D::CalcPositionMatrixMoveBack(float factor, const glm::vec3& modelSize)
{
	glm::vec3 originOffset = glm::vec3(std::max(std::max(modelSize.x, modelSize.y), modelSize.z) * -factor, 0, 0);
	cameraPos.Reset(originOffset, 0, 0);

	mvpUBO().model = glm::mat4(1.0f);
}

void VulkanApplication3D::CenterModel(Model& model)
{
	CalcPositionMatrixMoveBack(1.5f, model.GetModelSize());

	auto center = -(glm::abs(model.GetMaxExtent()) - glm::abs(model.GetMinExtent())) / 2.0f;
	mvpUBO().model = glm::translate(mvpUBO().model, glm::vec3(center.x, center.y, center.z));
}

void VulkanApplication3DLight::SetupLighting(glm::vec3 offset, float ambientLevel, float diffuseLevel, float specularLevel, float shinenessLevel, const glm::vec3& modelSize)
{
	lightUBO().lightPos = modelSize * offset;
	SetLightLevels(ambientLevel, diffuseLevel, specularLevel, shinenessLevel);
}

void VulkanApplication3DLight::SetLightLevels(float ambientLevel, float diffuseLevel, float specularLevel, float shinenessLevel)
{
	ambient = ambientLevel;
	diffuse = diffuseLevel;
	specular = specularLevel;
	shineness = shinenessLevel;
	UpdateLevels();
}

void VulkanApplication3DLight::UpdateLevels()
{
	lightUBO().ambientLevel = showAmbient ? ambient : 0.0f;
	lightUBO().diffuseLevel = showDiffuse ? diffuse : 0.0f;
	lightUBO().specularLevel = showSpecular ? specular : 0.0f;
	lightUBO().shineness = shineness;
}

void VulkanApplication3DSimpleLight::SetupLighting(UBO<UBO_lightToon>& lightToonUBO, glm::vec3 offset, std::array<float, 4> cutoff, std::array<float, 4> value, float brightFactor, const glm::vec3& modelSize)
{
	lightToonUBO().lightPos = modelSize * offset;
	VulkanPlayground::SetFloat4(lightToonUBO().cutoff, cutoff);
	std::transform(value.begin(), value.end(), value.begin(), [brightFactor](auto& val) { return val *= brightFactor; });
	VulkanPlayground::SetFloat4(lightToonUBO().value, value);
}

void VulkanApplication3D::AppProcessMouseMovement(const MouseData& mouseData)
{
	if (mouseData.mouseMode == MouseButton::Left)
		cameraPos.ProcessMouseMovement(mouseData.moveX, mouseData.moveY);
	else
		worldPos.ProcessMouseMovement(mouseData.moveX, mouseData.moveY);
	cameraPos.ProcessMouseScroll(mouseData.scroll);
}

void VulkanApplication3D::UpdateCamera(float frameTime, EventData& eventData)
{
	// Camera controls
	if (eventData.KeyDown('W'))
		cameraPos.ProcessKeyboardInput(CameraOrientator::UP, frameTime);
	if (eventData.KeyDown('S'))
		cameraPos.ProcessKeyboardInput(CameraOrientator::DOWN, frameTime);
	if (eventData.KeyDown('A'))
		cameraPos.ProcessKeyboardInput(CameraOrientator::LEFT, frameTime);
	if (eventData.KeyDown('D'))
		cameraPos.ProcessKeyboardInput(CameraOrientator::RIGHT, frameTime);
}

void VulkanApplication3D::AppResetScene()
{
	cameraPos.Reset(glm::vec3(), 0, 0);
	worldPos.Reset(glm::vec3(), 0, 0);
	mvpUBO().model = glm::mat4(1.0f);
}

void VulkanApplication3DLight::AppResetScene()
{
	showAmbient = showDiffuse = showSpecular = true;
	VulkanApplication3D::AppResetScene();
}

void VulkanApplication::CheckResetScene()
{
	if (resetScene)
	{
		AppResetScene();
		ResetScene();
		resetScene = false;
	}
}

void VulkanApplication::AppProcessKeyPresses(const EventData& eventData)
{
	if (eventData.KeyPressed(GLFW_KEY_TAB))
	{
		GetTextHelper()->ToggleShowingText();
	}
	if (eventData.KeyPressed('P'))
	{
		if (eventData.ShiftPressed())
		{
			vSync = !vSync;
			RecreateObjects();
		}
		else
			showFPS = !showFPS;
	}
	if (eventData.KeyPressed('R'))
	{
		resetScene = true;
	}
	ProcessKeyPresses(eventData);
}

void VulkanApplication::AppProcessMouseMovement(const MouseData& mouseData)
{
	ProcessMouseMovement(mouseData);
}

void VulkanApplication::ResetPrints(bool clear)
{
	GetTextHelper()->ResetPrints(clear);
}

void VulkanApplication::ProcessPrints(VulkanSystem& system, VkExtent2D workingExtent)
{
	std::set<Vulkan2DFont*> changedFonts;
	bool redraw = false, recreate = false;
	GetTextHelper()->Process(system, changedFonts, &redraw, &recreate);
	if (recreate)
	{
		RecreateObjects();
		return;	// Unexpected characters found - need to recreate font texture etc.
	}
	if (redraw)
		RedrawScene();

	for (auto font : changedFonts)
		font->Setup(system, *this, workingExtent, *fontRenderPass, windowWidth, windowHeight);
}

bool VulkanApplication::ObjectsCreated(VulkanSystem &system, RenderPasses& renderPasses)
{
	if (renderPasses.size() > 0)
	{
		for (auto pipeline : pipelines)
		{
			if (pipeline->CheckRecreate(system, renderPasses.GetScreenRenderPass()))
				RedrawScene();
		}
	}

	return objectsCreated; 
}

TypeSetter& VulkanApplication::PrintString(TypeSetter& typeSetter, const UnicodeString& unicodeString, glm::vec3 fontColour, float scale)
{
	return GetTextHelper()->PrintString(typeSetter, unicodeString, fontColour, scale);
}

TypeSetter& VulkanApplication::PrintString(int x, int y, const UnicodeString& unicodeString, glm::vec3 fontColour, float scale, unsigned int width, unsigned int height)
{
	return PrintString(GetTextHelper()->GetFont(), x, y, unicodeString, fontColour, scale, width, height);
}

TypeSetter& VulkanApplication::PrintString(Vulkan2DFont& font, int x, int y, const UnicodeString& unicodeString, glm::vec3 fontColour, float scale, unsigned int width, unsigned int height)
{
	return PrintString(font.GetTypeSetter(x, y, width, height), unicodeString, fontColour, scale);
}

void VulkanApplication::AppUpdateScene(VulkanSystem& system, const FPSTimer& frameTime)
{
	if (showFPS)
	{
		auto fpsString = frameTime.LatestFPS();
		if (vSync)
			fpsString += " (vSync)";
		if (GetTextHelper()->ShowingText())
			PrintString(10, 30, fpsString);
		else
		{
			static std::string lastFpsString;
			if (fpsString != lastFpsString)
			{
				lastFpsString = fpsString;
				std::cout << fpsString << "\n";
			}
		}
	}

	GetTextHelper()->PrintText(*this);

	UpdateScene(system, frameTime.LastFrameTime());
}

void VulkanApplication::SetTextHelper(ITextHelper* helper)
{
	textHelper.reset(helper);
}

ITextHelper* VulkanApplication::GetTextHelper()
{
	if (!textHelper)
		textHelper.reset(new TextHelperBase());

	return textHelper.get();
}
