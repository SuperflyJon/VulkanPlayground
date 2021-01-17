#pragma once

#include <chrono>
#include <iomanip>
#include <sstream>

class FPSTimer
{
public:
	FPSTimer()
	{
		detailed = false;
		lastFrameTime = 0;
		ResetTotals();
		frame_start = std::chrono::high_resolution_clock::now();
		dataLine = "fps:";
	}
	void ResetTotals()
	{
		frame_min = 0;
		frame_max = 0;
		numFrames = 0;
		frame_total = 0;
	}

	void Sample()
	{
		auto frame_end = std::chrono::high_resolution_clock::now();
		lastFrameTime = std::chrono::duration<float, std::chrono::seconds::period>(frame_end - frame_start).count();

		if (lastFrameTime > frame_max)
			frame_max = lastFrameTime;
		else if (frame_min == 0 || lastFrameTime < frame_min)
			frame_min = lastFrameTime;
		frame_total += lastFrameTime;

		numFrames++;

		if (frame_total > 1)
		{
			auto lastFPS = numFrames - 1;
			float avgFrameTime = ((float)frame_total / lastFPS);
			float delta = std::max(avgFrameTime - frame_min, frame_max - avgFrameTime);
			std::stringstream ss;
			ss << "fps: " << lastFPS;
			if (detailed)
				ss << " (" << std::fixed << std::setprecision(1) << avgFrameTime * 1000.0f << "ms +/- " << delta * 1000.0f << ")";
			dataLine = ss.str();
			ResetTotals();
		}

		frame_start = frame_end;	// Start of next frame
	}
	float LastFrameTime() const { return lastFrameTime; }

	std::string LatestFPS() const
	{
		return dataLine;
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> frame_start;
	float frame_min, frame_max, frame_total, lastFrameTime;
	int numFrames;
	bool detailed;
	std::string dataLine;
};
