#pragma once

#include "GLFW.h"
#include "Extensions.h"
#include "System.h"

class WindowSystem
{
public:
	WindowSystem() : instance(0), windowSurface(0) {}
	bool InitWindow(int windowWidth, int windowHeight, const std::string& windowName, GLFW::keyHandlerFn keyHandler = nullptr, void* keyHandlerData = nullptr);
	void RunApp(VulkanApplication& app);
	void Close();
	bool Running() { return window.Running(); }

	static void RunWindowed(int width, int height, const std::string& name, VulkanApplication& app);

	VulkanSystem& GetVulkanSystem() { return system; }

private:
	VkInstance instance;
	VkSurfaceKHR windowSurface;
	Extensions extensions;
	GLFW window;
	VulkanSystem system;
};
