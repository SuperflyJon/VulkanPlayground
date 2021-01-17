#pragma once

enum class MouseButton { None, Left, Right };

struct MouseData
{
	MouseData() : scroll(0), moveX(0), moveY(0), mouseMode(MouseButton::None) {}

	float scroll;
	float moveX;
	float moveY;
	MouseButton mouseMode;
};

class EventData
{
	enum KEY_STATE { KEY_DOWN = 1, KEY_UP = 0, KEY_PRESS = 2  };

public:
	EventData() : keys{  }, newKeyPress(false), shifted(false), firstMouse(true), lastX(0), lastY(0) {}

	bool KeyPressed(int key) const;
	bool KeyDown(int key) const;

	void UpdateKey(int key, int action);
	bool NewKeyPress();
	bool ShiftPressed() const { return shifted; }

	void MouseMoved(float xpos, float ypos);
	void MousePress(int button, bool press);
	void MouseScroll(float yoffset) { mouseData.scroll = yoffset; }

	bool GetMouseData(MouseData& mouseData);
		
private:
	mutable KEY_STATE keys[1024];
	bool newKeyPress;
	bool shifted;

	bool firstMouse;
	float lastX, lastY;

	MouseData mouseData;
};
