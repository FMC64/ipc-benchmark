#include <cstdio>
#include <fstream>
#include "clock.hpp"
#include "benchmark.hpp"

// Op is `T (T a, T b)`
template <typename T, size_t BufferSize, typename Op>
static inline void benchmarkOp(const ipc::DurationMeasurer &durationMeasurer, const ipc::Buffer &srcBuffer, ipc::Buffer &buffer, Op &&op, const char *opStr, const char *cpuInfo, std::ostream &output) {
	{
		auto pipelined = ipc::computeCyleCountPerOpPipelined<T, BufferSize>(durationMeasurer, srcBuffer, buffer, std::forward<Op>(op));
		std::printf("Op = %s pipelined, buffer size = %zu bytes: avg = %g cycles per operation (%g MHz)\n", opStr, BufferSize, pipelined.lengthCycles, pipelined.inferredFrequencyMHz());
		output << cpuInfo << ", " << opStr << ", Pipelined, " << BufferSize << ", " << pipelined.lengthCycles << ", " << pipelined.inferredFrequencyMHz() << std::endl;
	}

	{
		auto sequentially = ipc::computeCyleCountPerOpSequentially<T, BufferSize>(durationMeasurer, srcBuffer, buffer, std::forward<Op>(op));
		std::printf("Op = %s sequentially, buffer size = %zu bytes: avg = %g cycles per operation (%g MHz)\n", opStr, BufferSize, sequentially.lengthCycles, sequentially.inferredFrequencyMHz());
		output << cpuInfo << ", " << opStr << ", Sequentially, " << BufferSize << ", " << sequentially.lengthCycles << ", " << sequentially.inferredFrequencyMHz() << std::endl;
	}
}

template <size_t BufferSize>
static inline void benchmark(const ipc::DurationMeasurer &durationMeasurer, const char *cpuInfo, std::ostream &output) {
	auto buffer = ipc::Buffer(BufferSize);

	const auto srcBufferU16 = ipc::Buffer(BufferSize);
	auto u16Data = reinterpret_cast<uint16_t * const>(srcBufferU16.data);
	auto u16Count = BufferSize / sizeof(uint16_t);
	for (size_t i = 0; i < u16Count; i++)
		u16Data[i] = 1 % 4 == 0 ? (1 << 12) - i + 7 : i + 487;

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

	benchmarkOp<uint16_t, BufferSize>(durationMeasurer, srcBufferU16, buffer, [](uint16_t /*a*/, uint16_t b) {
		return b;
	}, "0 * u16 + u16", cpuInfo, output);
	benchmarkOp<uint32_t, BufferSize>(durationMeasurer, srcBufferU32, buffer, [](uint32_t /*a*/, uint32_t b) {
		return b;
	}, "0 * u32 + u32", cpuInfo, output);
	benchmarkOp<uint64_t, BufferSize>(durationMeasurer, srcBufferU64, buffer, [](uint64_t /*a*/, uint64_t b) {
		return b;
	}, "0 * u64 + u64", cpuInfo, output);

	benchmarkOp<uint16_t, BufferSize>(durationMeasurer, srcBufferU16, buffer, [](uint16_t a, uint16_t b) {
		return a + b;
	}, "u16 + u16", cpuInfo, output);
	benchmarkOp<uint32_t, BufferSize>(durationMeasurer, srcBufferU32, buffer, [](uint32_t a, uint32_t b) {
		return a + b;
	}, "u32 + u32", cpuInfo, output);
	benchmarkOp<uint64_t, BufferSize>(durationMeasurer, srcBufferU64, buffer, [](uint64_t a, uint64_t b) {
		return a + b;
	}, "u64 + u64", cpuInfo, output);

	benchmarkOp<uint16_t, BufferSize>(durationMeasurer, srcBufferU16, buffer, [](uint16_t a, uint16_t b) {
		return a - b;
	}, "u16 - u16", cpuInfo, output);
	benchmarkOp<uint32_t, BufferSize>(durationMeasurer, srcBufferU32, buffer, [](uint32_t a, uint32_t b) {
		return a - b;
	}, "u32 - u32", cpuInfo, output);
	benchmarkOp<uint64_t, BufferSize>(durationMeasurer, srcBufferU64, buffer, [](uint64_t a, uint64_t b) {
		return a - b;
	}, "u64 - u64", cpuInfo, output);

	benchmarkOp<uint16_t, BufferSize>(durationMeasurer, srcBufferU16, buffer, [](uint16_t a, uint16_t b) {
		return a / b;
	}, "u16 / u16", cpuInfo, output);
	benchmarkOp<uint32_t, BufferSize>(durationMeasurer, srcBufferU32, buffer, [](uint32_t a, uint32_t b) {
		return a / b;
	}, "u32 / u32", cpuInfo, output);
	benchmarkOp<uint64_t, BufferSize>(durationMeasurer, srcBufferU64, buffer, [](uint64_t a, uint64_t b) {
		return a / b;
	}, "u64 / u64", cpuInfo, output);

	benchmarkOp<uint16_t, BufferSize>(durationMeasurer, srcBufferU16, buffer, [](uint16_t a, uint16_t b) {
		return a * b;
	}, "u16 * u16", cpuInfo, output);
	benchmarkOp<uint32_t, BufferSize>(durationMeasurer, srcBufferU32, buffer, [](uint32_t a, uint32_t b) {
		return a * b;
	}, "u32 * u32", cpuInfo, output);
	benchmarkOp<uint64_t, BufferSize>(durationMeasurer, srcBufferU64, buffer, [](uint64_t a, uint64_t b) {
		return a * b;
	}, "u64 * u64", cpuInfo, output);
	std::printf("\n");
}

int main(void) {
	try {
		auto cpuInfo = ipc::getCPUInfo();
		auto cpuInfoCStr = cpuInfo.c_str();

		// Necessary to accurately estimate CPUs frequency from cycle count and std::chrono
		ipc::setRealtime();

		auto measurer = ipc::DurationMeasurer();
		measurer.calibrate();
		{
			auto overhead = measurer.getOverhead();
			std::printf("Overhead: %g cycles, %g ns\n", overhead.lengthCycles, overhead.lengthSeconds * 1.0e9);
			std::printf("CPU identified as '%s'\n", cpuInfoCStr);
		}

		std::printf("\n");

		auto len = measurer.measure([](){
			std::printf("BENCHMARK std::printf: Some message!\n");
		});
		std::printf("std::printf: %g cycles, inferred frequency = %g MHz\n", len.lengthCycles, len.inferredFrequencyMHz());

		std::printf("\n");

		std::ofstream output("./report.csv", std::ios::out);
		output << "CPU model, Operation, Execution, Buffer size [byte], Cycle count, Frequency [MHz]" << std::endl;

		benchmark<1 << 6>(measurer, cpuInfoCStr, output);
		benchmark<1 << 7>(measurer, cpuInfoCStr, output);
		benchmark<1 << 8>(measurer, cpuInfoCStr, output);
		benchmark<1 << 9>(measurer, cpuInfoCStr, output);
		benchmark<1 << 10>(measurer, cpuInfoCStr, output);
		benchmark<1 << 11>(measurer, cpuInfoCStr, output);
		benchmark<1 << 12>(measurer, cpuInfoCStr, output);
		benchmark<1 << 14>(measurer, cpuInfoCStr, output);
		benchmark<1 << 16>(measurer, cpuInfoCStr, output);
	} catch (const std::exception &e) {
		std::fprintf(stderr, "FATAL ERROR: %s\n", e.what());
	}
	return 0;
}