#include <cstdio>
#include "clock.hpp"
#include "benchmark.hpp"

// Op is `T (T a, T b)`
template <typename T, size_t BufferSize, typename Op>
static inline void benchmarkOp(const ipc::DurationMeasurer &durationMeasurer, const ipc::Buffer &srcBuffer, ipc::Buffer &buffer, Op &&op, const char *opStr) {
	auto cycleCount = ipc::computeCyleCountPerOpPipelined<T, BufferSize>(durationMeasurer, srcBuffer, buffer, std::forward<Op>(op));
	std::printf("Op = %s pipelined, buffer size = %zu bytes: avg = %g cycles per operation\n", opStr, BufferSize, cycleCount);
}

template <size_t BufferSize>
static inline void benchmark(const ipc::DurationMeasurer &durationMeasurer) {
	auto buffer = ipc::Buffer(BufferSize);

	const auto srcBufferU32 = ipc::Buffer(BufferSize);
	auto u32Data = reinterpret_cast<uint32_t * const>(srcBufferU32.data);
	auto u32Count = BufferSize / sizeof(uint32_t);
	for (size_t i = 0; i < u32Count; i++)
		u32Data[i] = 1 % 4 == 0 ? (1 << 20) - i + 7 : i + 16487;

	const auto srcBufferU64 = ipc::Buffer(BufferSize);
	auto u64Data = reinterpret_cast<uint64_t * const>(srcBufferU64.data);
	auto u64Count = BufferSize / sizeof(uint64_t);
	for (size_t i = 0; i < u64Count; i++)
		u64Data[i] = 1 % 4 == 0 ? (static_cast<size_t>(1) << 42) - i + 7 : i + 16487;

	benchmarkOp<uint32_t, BufferSize>(durationMeasurer, srcBufferU32, buffer, [](uint32_t a, uint32_t b) {
		return a / b;
	}, "u32 / u32");
	benchmarkOp<uint64_t, BufferSize>(durationMeasurer, srcBufferU64, buffer, [](uint64_t a, uint64_t b) {
		return a / b;
	}, "u64 / u64");
	benchmarkOp<uint32_t, BufferSize>(durationMeasurer, srcBufferU32, buffer, [](uint32_t a, uint32_t b) {
		return a * b;
	}, "u32 * u32");
	benchmarkOp<uint64_t, BufferSize>(durationMeasurer, srcBufferU64, buffer, [](uint64_t a, uint64_t b) {
		return a * b;
	}, "u64 * u64");
	std::printf("\n");
}

int main(void) {
	try {
		// Necessary to accurately estimate CPUs frequency from cycle count and std::chrono
		ipc::setRealtime();

		auto measurer = ipc::DurationMeasurer();
		measurer.calibrate();
		{
			auto overhead = measurer.getOverhead();
			std::printf("Overhead: %g cycles, %g ns\n", overhead.lengthCycles, overhead.lengthSeconds * 1.0e9);
		}

		std::printf("\n");

		auto len = measurer.measure([](){
			std::printf("BENCHMARK std::printf: Some message!\n");
		});
		std::printf("std::printf: %g cycles, inferred frequency = %g MHz\n", len.lengthCycles, len.inferredFrequencyMHz());

		std::printf("\n");

		benchmark<1 << 6>(measurer);
		benchmark<1 << 7>(measurer);
		benchmark<1 << 8>(measurer);
		benchmark<1 << 9>(measurer);
		benchmark<1 << 10>(measurer);
		benchmark<1 << 11>(measurer);
		benchmark<1 << 12>(measurer);
		benchmark<1 << 16>(measurer);
	} catch (const std::exception &e) {
		std::fprintf(stderr, "FATAL ERROR: %s\n", e.what());
	}
	return 0;
}