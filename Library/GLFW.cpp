
#include "stdafx.h"

#include "GLFW.h"
// Include GLFW (Vulkan version)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "SwapChain.h"
#include "RenderPass.h"
#include "Application.h"

namespace glfwCallbacks
{
	// Is called whenever a key is pressed/released via GLFW
	void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mode*/)
	{
		GLFW* glfw = static_cast<GLFW *>(glfwGetWindowUserPointer(window));
		if (glfw != nullptr)
			glfw->GetEventData().UpdateKey(key, action);
	}
	void mouse_callback(GLFWwindow* window, double xpos, double ypos)
	{
		GLFW* glfw = static_cast<GLFW*>(glfwGetWindowUserPointer(window));
		if (glfw != nullptr)
			glfw->GetEventData().MouseMoved((float)xpos, (float)ypos);
	}
	void mouse_press_callback(GLFWwindow* window, int button, int action, int /*modkeys*/)
	{
		GLFW* glfw = static_cast<GLFW*>(glfwGetWindowUserPointer(window));
		if (glfw != nullptr)
			glfw->GetEventData().MousePress(button, action == GLFW_PRESS);
	}
	void scroll_callback(GLFWwindow* window, double /*xoffset*/, double yoffset)
	{
		GLFW* glfw = static_cast<GLFW*>(glfwGetWindowUserPointer(window));
		if (glfw != nullptr)
			glfw->GetEventData().MouseScroll((float)yoffset);
	}
}

GLFW::GLFW()
	: exitApplication(false), window(nullptr), keyHandler(nullptr), keyHandlerData(nullptr), fullScreen(false), windowX(100), windowY(100), windowedWidth(0), windowedHeight(0)
{
}

void GLFW::Create(int width, int height, const std::string& name, keyHandlerFn keyHandlerVal, void* keyHandlerDataVal, bool showWindow)
{
	keyHandler = keyHandlerVal;
	keyHandlerData = keyHandlerDataVal;
	// Create a window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	if (!showWindow)
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	if (fullScreen)
	{
		windowedWidth = width, windowedHeight = height;
		auto monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		window = glfwCreateWindow(mode->width, mode->height, name.c_str(), monitor, nullptr);
	}
	else
		window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);

	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, glfwCallbacks::key_callback);
	glfwSetCursorPosCallback(window, glfwCallbacks::mouse_callback);
	glfwSetMouseButtonCallback(window, glfwCallbacks::mouse_press_callback);
	glfwSetScrollCallback(window, glfwCallbacks::scroll_callback);

//		glfwSetWindowUserPointer(window, this);	// Grab cursor
//		glfwSetWindowSizeCallback(window, OnWindowResized);	// Not needed
}


void* GLFW::GetCurrentMonitor()
{
	int bestoverlap = -1;
	GLFWmonitor* bestmonitor = nullptr;

	int numMonitors;
	GLFWmonitor** monitors = glfwGetMonitors(&numMonitors);

	for (int i = 0; i < numMonitors; i++) {
		auto mode = glfwGetVideoMode(monitors[i]);
		int monitorWidth = mode->width, monitorHeight = mode->height;
		int monitorX, monitorY;
		glfwGetMonitorPos(monitors[i], &monitorX, &monitorY);

		int overlap = std::max(0, std::min(windowX + windowedWidth, monitorX + monitorWidth) - std::max(windowX, monitorX))
					* std::max(0, std::min(windowY + windowedHeight, monitorY + monitorHeight) - std::max(windowY, monitorY));

		if (overlap > bestoverlap)
		{
			bestoverlap = overlap;
			bestmonitor = monitors[i];
		}
	}

	return bestmonitor;
}

void GLFW::ToggleFullscreen()
{
	fullScreen = !fullScreen;

	if (fullScreen)
	{
		glfwGetWindowPos(window, &windowX, &windowY);
		GetWindowSize(&windowedWidth, &windowedHeight);
		auto monitor = (GLFWmonitor*)GetCurrentMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
	}
	else
	{
		glfwSetWindowMonitor(window, nullptr, windowX, windowY, windowedWidth, windowedHeight, GLFW_DONT_CARE);
	}
}

std::vector<const char*> GLFW::GetReqExtensions()
{
	GLFW::Init();

#define SKIP_SLOW_CHECK	// GLFW slow to return required extensions
#ifdef SKIP_SLOW_CHECK
	return { "VK_KHR_surface", "VK_KHR_win32_surface" };
#else
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	return { glfwExtensions, glfwExtensions + glfwExtensionCount };
#endif
}

VkSurfaceKHR GLFW::CreateSurface(VkInstance instance)
{
	VkSurfaceKHR surface;
	CHECK_VULKAN(glfwCreateWindowSurface(instance, window, nullptr, &surface), "Failed to create window surface!");
	if (VulkanPlayground::showObjectCreationMessages)
		std::cout << "Window Surface created (" << surface << ")\n";
	return surface;
}

void GLFW::DestroySurface(VkInstance instance, VkSurfaceKHR windowSurface)
{
	vkDestroySurfaceKHR(instance, windowSurface, nullptr);
}

void GLFW::Init()
{
	glfwInit();
}

bool GLFW::PollEvents(MouseData& mouseData)
{
	glfwPollEvents();
	return eventData.GetMouseData(mouseData);
}

void GLFW::Terminate()
{
	glfwTerminate();
}

void GLFW::Show()
{
	glfwShowWindow(window);
}

void GLFW::Hide()
{
	glfwHideWindow(window);
}

bool GLFW::Running()
{
	return !exitApplication && !glfwWindowShouldClose(window);
}

void GLFW::GetWindowSize(int* width, int* height)
{
	glfwGetWindowSize(window, width, height);
}

void GLFW::Close()
{
	glfwHideWindow(window);
	glfwSetWindowShouldClose(window, true);
}

void GLFW::Tidy()
{
	glfwDestroyWindow(window);
}

bool GLFW::CheckFoKeyPresses(VulkanApplication& app)
{
	if (eventData.NewKeyPress())
	{
		app.AppProcessKeyPresses(eventData);
		if (keyHandler && keyHandler(*this, keyHandlerData))
			return true;

		if (KeyPressed(GLFW_KEY_ESCAPE))
			exitApplication = true;

		if (KeyPressed(GLFW_KEY_F11))
		{
			ToggleFullscreen();
			return true;
		}
	}
	return false;
}

bool GLFW::KeyPressed(int key) const
{
	return eventData.KeyPressed(key);
}
