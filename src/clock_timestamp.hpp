#pragma once

#ifdef _WIN32

#include <intrin.h>

#else

#include <x86intrin.h>

#endif

#include <cstdint>

namespace ipc {

static inline size_t getClockTimestamp(void) {
	return __rdtsc();
}

}