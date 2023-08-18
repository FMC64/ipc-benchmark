#include <cstdio>
#include "clock.hpp"

int main(void) {
	auto measurer = ipc::DurationMeasurer();
	measurer.calibrate();
	auto len = measurer.measure([](){
		std::printf("Some message!\n");
	});
	std::printf("Last message needed %zu cycles to complete, inferred freq = %g MHz\n", len.lengthCycles, len.inferredFrequencyMHz());
	return 0;
}