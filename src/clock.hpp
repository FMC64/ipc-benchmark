#pragma once

#ifdef _WIN32

#include <intrin.h>
#include <windows.h>

#else

#include <x86intrin.h>
#include <unistd.h>
#include <string.h>

#endif

#include <cstdint>
#include <chrono>
#include <stdexcept>
#include <sstream>

namespace ipc {

#ifdef _WIN32

// Largely adapted from https://stackoverflow.com/a/17387176
// Inefficient but who cares, everything is already going down if that gets called
static inline std::string winGetLastError(void) {
	//Get the error message ID, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0) {
		return std::string(); //No error message has been recorded
	}

	LPSTR messageBuffer = nullptr;

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL
	);

	//Copy the error message into a std::string.
	std::string message(messageBuffer, size);

	//Free the Win32's string's buffer.
	LocalFree(messageBuffer);
	return message;
}

#endif

static inline void setRealtime(void) {
#ifdef _WIN32

	auto res = SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	if (!res) {
		std::stringstream ss;
		ss << "setRealtime(_WIN32): " << winGetLastError();
		throw std::runtime_error(ss.str());
	}

#else

	auto res = nice(-20);
	if (res == -1) {
		std::stringstream ss;
		ss << "setRealtime(unistd.h): " << strerror(errno);
		throw std::runtime_error(ss.str());
	}

#endif
}

static inline size_t getClockTimestamp(void) {
	return __rdtsc();
}

struct Duration {
	size_t lengthCycles;
	double lengthSeconds;

	double inferredFrequency(void) const {
		return static_cast<double>(lengthCycles) / lengthSeconds;
	}

	double inferredFrequencyMHz(void) const {
		return inferredFrequency() / 1.0e6;
	}
};

class DurationMeasurer
{
	// How much time does sampling take
	Duration m_overhead;

	// How many dummy samplings to average to calibrate
	static inline constexpr size_t calibrationIterationCount = 1 << 16;

public:
	DurationMeasurer(void) :
		m_overhead({
			.lengthCycles = 0,
			.lengthSeconds = 0.0
		})
	{
	}

	// Only informative, do not use these timings yourself, measure already does the compensation by default
	Duration getOverhead(void) const {
		return m_overhead;
	}

	template <typename Fn>
	Duration measure(Fn &&fn, bool compensate = true) const {
		auto beginChrono = std::chrono::high_resolution_clock::now();
		volatile auto begin = getClockTimestamp();
		fn();
		volatile auto end = getClockTimestamp();
		auto endChrono = std::chrono::high_resolution_clock::now();

		auto res = Duration{
			.lengthCycles = end - begin,
			.lengthSeconds = std::chrono::duration<double>(endChrono - beginChrono).count()
		};
		if (compensate) {
			if (res.lengthCycles > m_overhead.lengthCycles)
				res.lengthCycles -= m_overhead.lengthCycles;
			else
				res.lengthCycles = 0;

			if (res.lengthSeconds > m_overhead.lengthSeconds)
				res.lengthSeconds -= m_overhead.lengthSeconds;
			else
				res.lengthSeconds = 0;
		}
		return res;
	}

private:
	Duration computeCalibration(void) const {
		Duration durations[calibrationIterationCount];

		// Warmup batch
		for (size_t i = 0; i < calibrationIterationCount; i++)
			durations[i] = measure([](){}, false);

		// Actual data
		for (size_t i = 0; i < calibrationIterationCount; i++)
			durations[i] = measure([](){}, false);

		Duration res = {
			.lengthCycles = 0,
			.lengthSeconds = 0.0
		};
		for (size_t i = 0; i < calibrationIterationCount; i++) {
			auto cur = durations[i];
			res.lengthCycles += cur.lengthCycles;
			res.lengthSeconds += cur.lengthSeconds;
		}
		res.lengthCycles /= calibrationIterationCount;
		res.lengthSeconds /= static_cast<double>(calibrationIterationCount);
		return res;
	}

public:
	void calibrate(void) {
		m_overhead = computeCalibration();
	}
};

}