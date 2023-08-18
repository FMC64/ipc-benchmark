#include <cstdio>
#include "clock.hpp"

int main(void) {
	try {
		// Necessary to accurately estimate CPUs frequency from cycle count and std::chrono
		ipc::setRealtime();

		auto measurer = ipc::DurationMeasurer();
		measurer.calibrate();

		auto len = measurer.measure([](){
			std::printf("Some message!\n");
		});

		std::printf("Last message needed %zu cycles to complete, inferred freq = %g MHz\n", len.lengthCycles, len.inferredFrequencyMHz());
	} catch (const std::exception &e) {
		std::fprintf(stderr, "FATAL ERROR: %s\n", e.what());
	}
	return 0;
}