#include <cstdio>
#include "clock_timestamp.hpp"

int main(void) {
	auto begin = ipc::getClockTimestamp();
	std::printf("Some message!\n");
	auto end = ipc::getClockTimestamp();
	std::printf("Last message needed %zu cycles to complete\n", end - begin);
	return 0;
}