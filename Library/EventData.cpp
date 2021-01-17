
#include "stdafx.h"
#include "EventData.h"

constexpr auto KEY_LEFT_SHIFT = 340;
constexpr auto KEY_RIGHT_SHIFT = 344;

void EventData::UpdateKey(int key, int action)
{
	if (key >= 0 && key < 1024)
	{
		if (action == KEY_DOWN)
		{
			if (keys[key] != KEY_DOWN)
			{
				keys[key] = KEY_PRESS;
				if (key == KEY_LEFT_SHIFT || key == KEY_RIGHT_SHIFT)
					shifted = true;
				else
					newKeyPress = true;
			}
		}
		else if (action == KEY_UP)
		{
			keys[key] = KEY_UP;
			if (key == KEY_LEFT_SHIFT || key == KEY_RIGHT_SHIFT)
				shifted = false;
		}
	}
}

bool EventData::NewKeyPress()
{
	if (newKeyPress)
	{
		newKeyPress = false;
		return true;
	}
	return false;
}

bool EventData::KeyPressed(int key) const
{
	if (keys[key] == KEY_PRESS)
	{
		keys[key] = KEY_DOWN;
		return true;
	}
	else
	{
		return false;
	}
}

bool EventData::KeyDown(int key) const
{
	return (keys[key] == KEY_PRESS);
}

void EventData::MousePress(int button, bool press)
{
	if (press)
	{
		if (button == 0)
			mouseData.mouseMode = MouseButton::Left;
		else if (button == 1)
			mouseData.mouseMode = MouseButton::Right;
	}
	else
	{
		mouseData.mouseMode = MouseButton::None;
		firstMouse = true;
	}
}

void EventData::MouseMoved(float xpos, float ypos)
{
	if (mouseData.mouseMode == MouseButton::None)
		return;

	if (firstMouse)
	{
		lastX = (float)xpos;
		lastY = (float)ypos;
		firstMouse = false;
	}

	mouseData.moveX = (float)xpos - lastX;
	mouseData.moveY = (float)ypos - lastY;

	lastX = (float)xpos;
	lastY = (float)ypos;
}

bool EventData::GetMouseData(MouseData& data)
{
	if (mouseData.moveX || mouseData.moveY || mouseData.scroll)
	{
		data = mouseData;
		mouseData.moveX = mouseData.moveY = mouseData.scroll = 0;
		return true;
	}
	else
		return false;
}
