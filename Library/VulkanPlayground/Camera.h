#pragma once

class CameraOrientator
{
private:
	glm::vec3 position;

	// Eular Angles
	float yaw;
	float pitch;

	// Default camera values
	const float SPEED = 5.0f;
	const float SENSITIVTY = 0.25f;
	const glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);

public:
	CameraOrientator() : yaw(0.0f), pitch(0.0f)
	{
	}

	enum Camera_Movement { UP, DOWN, LEFT, RIGHT };

	const glm::vec3& GetPosition() const { return position; }
	glm::vec3& GetPosition() { return position; }
	void SetPosition(const glm::vec3& val) { position = val; }
	void SetYaw(float value) { yaw = value; }
	void SetPitch(float value) { pitch = value; }

	// Returns the view matrix calculated using Eular Angles and the LookAt Matrix
	glm::mat4 CalcViewMatrix() const;
	void Reset(glm::vec3 newPosition, float newYaw, float newPitch);
	// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void ProcessKeyboardInput(Camera_Movement direction, float deltaTime);
	// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
	// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis (y axis)
	void ProcessMouseScroll(float yoffset);
	// Calculates the front vector from the Camera's Eular Angles
	glm::vec3 CalculateFrontVector() const;
	std::string GetDebugString() const;
};
