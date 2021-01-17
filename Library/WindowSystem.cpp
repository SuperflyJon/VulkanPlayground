
#include "stdafx.h"
#include "WindowSystem.h"
#include "WinUtil.h"
#include "Application.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "FrameTimer.h"

bool WindowSystem::InitWindow(int windowWidth, int windowHeight, const std::string& windowName, GLFW::keyHandlerFn keyHandler, void* keyHandlerData)
{
	try
	{
		extensions.Setup(GLFW::GetReqExtensions());
		instance = VulkanPlayground::CreateInstance(windowName, extensions);
		extensions.SetupDebugCallback(instance, VulkanPlayground::DebugCallbackFn);

		window.Create(windowWidth, windowHeight, windowName, keyHandler, keyHandlerData, false);

		windowSurface = window.CreateSurface(instance);

		system.FindDevice(instance, windowSurface, extensions);
	}
	catch (std::runtime_error & err)
	{
		WinUtils::OutputError("Failed to setup window, fatal error: " + std::string(err.what()));
		return false;
	}
	return true;
}

void WindowSystem::RunApp(VulkanApplication& app)
{
	app.Starting(system);
	SwapChain swapChain;
	RenderPasses renderPasses;

	try
	{
		system.Setup(extensions, app);
		// Next line crashes!  Bug in VulkanSDK and/or graphics driver?
		//system.DebugNameObject(instance, VK_OBJECT_TYPE_INSTANCE, "Main Vulkan Instance");	// This is here as need to create device before naming things

		app.GetWindowSize(window);

		FPSTimer fpsTimer;
		bool minimised = false, windowShown = false;

		while (Running())
		{
			MouseData mouseData;
			if (window.PollEvents(mouseData))
				app.AppProcessMouseMovement(mouseData);

			if (window.CheckFoKeyPresses(app))
				break;

			if (!app.ObjectsCreated(system, renderPasses))
				app.SetupWindowObjects(swapChain, windowSurface, system, renderPasses);

			app.CheckResetScene();
			if (!app.ObjectsCreated(system, renderPasses))
				continue;	// Loop around to recreate things

			app.UpdateCamera(fpsTimer.LastFrameTime(), window.GetEventData());
			app.ResetPrints(false);
			app.AppUpdateScene(system, fpsTimer);
			app.ProcessPrints(system, swapChain.GetWorkingExtent());

			if (!app.ObjectsCreated(system, renderPasses))
				continue;	// Loop around to recreate things

			if (app.RedrawSceneSet())
				app.CreateCommandBuffers(system, renderPasses);

			if (!windowShown)
			{	// Doing this here minimises the delay between showing the window and displaying the scene (avoids a blank white window)
				window.Show();
				windowShown = true;
			}

			bool okay = false;
			if (!minimised)
			{
				okay = swapChain.DrawFrame(system, renderPasses);
			}
			if (!okay)
			{
				app.GetWindowSize(window);
				minimised = (app.GetWindowWidth() == 0) || (app.GetWindowHeight() == 0);
				if (!minimised)
				{
					window.Hide(); window.Show();	// Workaround issue with GLFW resizing not working quite correctly... (TODO: Try latest GLFW and see if it stil goes wrong)
					app.RecreateObjects();
				}
			}
			fpsTimer.Sample();
		}
	}
	catch (std::runtime_error & err)
	{
		WinUtils::OutputError("Application terminated, fatal error: " + std::string(err.what()));
		window.ExitApplication();
	}
	system.DeviceWaitIdle();

	app.Tidy(system);

	swapChain.Tidy(system);
	renderPasses.Tidy(system);
}

void WindowSystem::Close()
{
	try
	{
		system.TidyUp();

		GLFW::DestroySurface(instance, windowSurface);
		extensions.TidyUpDebugCallback();
		vkDestroyInstance(instance, nullptr);

		window.Tidy();
		GLFW::Terminate();
	}
	catch (std::runtime_error & err)
	{
		WinUtils::OutputError("Failed to tidy up, fatal error: " + std::string(err.what()));
	}

#ifndef _DEBUG
	WinUtils::Pause();	// Prompt for pause
#endif
}

void WindowSystem::RunWindowed(int width, int height, const std::string& name, VulkanApplication& app)
{
	WindowSystem windowSystem;
	if (!windowSystem.InitWindow(width, height, name))
		return;

	app.SetTitle(name);
	do
	{
		windowSystem.RunApp(app);
	} while (windowSystem.Running());

	windowSystem.Close();
}
