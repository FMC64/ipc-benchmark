#pragma once

#ifdef _WIN32

#include <intrin.h>

#else

#include <x86intrin.h>

#endif

#include <cstdint>
#include <chrono>

namespace ipc {

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