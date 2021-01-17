#pragma once

#include <string>
#include "Common.h"
#include "EventData.h"

#define GLFW_KEY_RIGHT              262
#define GLFW_KEY_LEFT               263
#define GLFW_KEY_DOWN               264
#define GLFW_KEY_UP                 265
#define GLFW_KEY_ESCAPE             256
#define GLFW_KEY_ENTER              257
#define GLFW_KEY_TAB                258
#define GLFW_KEY_KP_ENTER           335

typedef struct GLFWwindow GLFWwindow;
class VulkanApplication;
class SwapChain;
class Extensions;

class GLFW
{
public:
	GLFW();

	typedef bool (*keyHandlerFn)(GLFW& window, void* data);
	void Create(int width, int height, const std::string& name, keyHandlerFn keyHandler, void* keyHandlerData, bool showWindow = true);
	void ToggleFullscreen();
	void Show();
	void Hide();
	bool Running();
	void GetWindowSize(int* width, int* height);
	void Tidy();
	void Close();

	bool CheckFoKeyPresses(VulkanApplication& app);
	bool KeyPressed(int key) const;

	static std::vector<const char *> GetReqExtensions();

	VkSurfaceKHR CreateSurface(VkInstance instance);
	static void DestroySurface(VkInstance instance, VkSurfaceKHR windowSurface);
	static void Init();
	bool PollEvents(MouseData& mouseData);
	static void Terminate();

	void ExitApplication() { exitApplication = true; }

	void RegisterKeyHandler(keyHandlerFn handler, void* data) { keyHandler = handler, keyHandlerData = data; }

	EventData& GetEventData() { return eventData; }

private:
	void* GetCurrentMonitor();

	bool exitApplication;
	GLFWwindow* window;
	EventData eventData;

	keyHandlerFn keyHandler;
	void *keyHandlerData;
	bool fullScreen;
	int windowX, windowY;
	int windowedWidth, windowedHeight;
};
