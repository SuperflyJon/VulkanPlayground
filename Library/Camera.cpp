#include "stdafx.h"
#include "Camera.h"

// Returns the view matrix calculated using Eular Angles and the LookAt Matrix
glm::mat4 CameraOrientator::CalcViewMatrix() const
{
	glm::vec3 front = CalculateFrontVector();
	glm::vec3 right = glm::normalize(glm::cross(front, WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	glm::vec3 up = glm::normalize(glm::cross(right, front));

	return glm::lookAt(position, position + front, up);
}

void CameraOrientator::Reset(glm::vec3 newPosition, float newYaw, float newPitch)
{
	position = newPosition;
	yaw = newYaw;
	pitch = newPitch;
}

// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
void CameraOrientator::ProcessKeyboardInput(Camera_Movement direction, float deltaTime)
{
	float velocity = SPEED * deltaTime;

	if (direction == UP)
		position -= WorldUp * velocity;
	if (direction == DOWN)
		position += WorldUp * velocity;
	if (direction == LEFT || direction == RIGHT)
	{
		glm::vec3 right = glm::normalize(glm::cross(CalculateFrontVector(), WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.

		if (direction == LEFT)
			position -= right * velocity;
		if (direction == RIGHT)
			position += right * velocity;
	}
}

// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void CameraOrientator::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
	xoffset *= SENSITIVTY;
	yoffset *= SENSITIVTY;

	yaw += xoffset;
	pitch += yoffset;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (constrainPitch)
	{
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;
	}
}

// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis (y axis)
void CameraOrientator::ProcessMouseScroll(float yoffset)
{
	position += CalculateFrontVector() * yoffset;
}

// Calculates the front vector from the Camera's Eular Angles
glm::vec3 CameraOrientator::CalculateFrontVector() const
{
	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	return glm::normalize(front);
}

std::string CameraOrientator::GetDebugString() const
{
	std::stringstream ss;
	ss << "Pos (" << position.x << ", " << position.y << ", " << position.z << ") Yaw: " << yaw << " Pitch: " << pitch;
	return ss.str();
}
