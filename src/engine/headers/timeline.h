#pragma once

#include <chrono>
#include <atomic>

class Timeline
{
public:
	Timeline(long long tickSizeMs = 16);

	void update();

	long long getElapsedTicks() const;
	float getDeltaTime() const;

    // Renamed for consistency with isPaused
	void pause();
	void unpause();
	bool isPaused() const;

    // Kept public for flexibility
	void setTimeScale(double scale);

    // Restored from engine branch
	void timeScaleUp();
	void timeScaleDown();

private:
	std::chrono::high_resolution_clock::time_point m_startTime;
	std::chrono::high_resolution_clock::time_point m_lastUpdateTime;
	std::chrono::high_resolution_clock::time_point m_pauseStartTime;

    // Restored std::atomic variables from engine branch for thread-safety
	std::atomic<long long> m_tickSizeNs;
	std::atomic<long long> m_totalElapsedNs;
	std::atomic<long long> m_deltaTimeNs;
	std::atomic<long long> m_totalPausedNs;
	std::atomic<double> m_timeScale;
	std::atomic<bool> m_isPaused;
};