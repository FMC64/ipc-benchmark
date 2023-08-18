#include <cstdio>
#include "clock.hpp"
#include "benchmark.hpp"

int main(void) {
	try {
		// Necessary to accurately estimate CPUs frequency from cycle count and std::chrono
		ipc::setRealtime();

		auto measurer = ipc::DurationMeasurer();
		measurer.calibrate();
		{
			auto overhead = measurer.getOverhead();
			std::printf("Overhead: %zu cycles, %g ns\n", overhead.lengthCycles, overhead.lengthSeconds * 1.0e9);
		}

		std::printf("\n");

		auto len = measurer.measure([](){
			std::printf("BENCHMARK std::printf: Some message!\n");
		});
		std::printf("std::printf: %zu cycles, inferred frequency = %g MHz\n", len.lengthCycles, len.inferredFrequencyMHz());

		std::printf("\n");

		auto buffer = ipc::Buffer(static_cast<size_t>(1) << 16);
	} catch (const std::exception &e) {
		std::fprintf(stderr, "FATAL ERROR: %s\n", e.what());
	}
	return 0;
}