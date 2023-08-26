#include <cstdio>
#include <fstream>
#include <cstdlib>
#include "clock.hpp"
#include "benchmark.hpp"
#include "data.hpp"

// Op is `T (T a, T b)`
template <typename T, size_t BufferSize, typename Op>
static inline void benchmarkOp(const ipc::DurationMeasurer &durationMeasurer, const ipc::Buffer &srcBuffer, ipc::Buffer &buffer, Op &&op, const char *opStr, const char *cpuInfo, std::ostream &output) {
	if constexpr (BufferSize <= 4096) {
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

static inline constexpr size_t maxBufferSize = 1 << 16;

static auto buffer = ipc::Buffer(maxBufferSize);
static auto srcBuffer = ipc::Buffer(maxBufferSize);

template <size_t BufferSize>
static inline void benchmark(const ipc::DurationMeasurer &durationMeasurer, const char *cpuInfo, std::ostream &output) {
	static_assert(BufferSize <= maxBufferSize, "BufferSize must not exceed maxBufferSize");

	ipc::writeU16Data(srcBuffer, BufferSize);
	benchmarkOp<uint16_t, BufferSize>(durationMeasurer, srcBuffer, buffer, [](uint16_t /*a*/, uint16_t b) {
		return b;
	}, "0 * u16 + u16", cpuInfo, output);
	ipc::writeU32Data(srcBuffer, BufferSize);
	benchmarkOp<uint32_t, BufferSize>(durationMeasurer, srcBuffer, buffer, [](uint32_t /*a*/, uint32_t b) {
		return b;
	}, "0 * u32 + u32", cpuInfo, output);
	ipc::writeU64Data(srcBuffer, BufferSize);
	benchmarkOp<uint64_t, BufferSize>(durationMeasurer, srcBuffer, buffer, [](uint64_t /*a*/, uint64_t b) {
		return b;
	}, "0 * u64 + u64", cpuInfo, output);

	ipc::writeU16Data(srcBuffer, BufferSize);
	benchmarkOp<uint16_t, BufferSize>(durationMeasurer, srcBuffer, buffer, [](uint16_t a, uint16_t b) {
		return a + b;
	}, "u16 + u16", cpuInfo, output);
	ipc::writeU32Data(srcBuffer, BufferSize);
	benchmarkOp<uint32_t, BufferSize>(durationMeasurer, srcBuffer, buffer, [](uint32_t a, uint32_t b) {
		return a + b;
	}, "u32 + u32", cpuInfo, output);
	ipc::writeU64Data(srcBuffer, BufferSize);
	benchmarkOp<uint64_t, BufferSize>(durationMeasurer, srcBuffer, buffer, [](uint64_t a, uint64_t b) {
		return a + b;
	}, "u64 + u64", cpuInfo, output);

	ipc::writeU16Data(srcBuffer, BufferSize);
	benchmarkOp<uint16_t, BufferSize>(durationMeasurer, srcBuffer, buffer, [](uint16_t a, uint16_t b) {
		return a - b;
	}, "u16 - u16", cpuInfo, output);
	ipc::writeU32Data(srcBuffer, BufferSize);
	benchmarkOp<uint32_t, BufferSize>(durationMeasurer, srcBuffer, buffer, [](uint32_t a, uint32_t b) {
		return a - b;
	}, "u32 - u32", cpuInfo, output);
	ipc::writeU64Data(srcBuffer, BufferSize);
	benchmarkOp<uint64_t, BufferSize>(durationMeasurer, srcBuffer, buffer, [](uint64_t a, uint64_t b) {
		return a - b;
	}, "u64 - u64", cpuInfo, output);

	/*ipc::writeU16Data(srcBuffer, BufferSize);
	benchmarkOp<uint16_t, BufferSize>(durationMeasurer, srcBuffer, buffer, [](uint16_t a, uint16_t b) {
		return a / b;
	}, "u16 / u16", cpuInfo, output);*/
	ipc::writeU32Data(srcBuffer, BufferSize);
	ipc::convU32ToF32(srcBuffer, BufferSize);
	benchmarkOp<float, BufferSize>(durationMeasurer, srcBuffer, buffer, [](float a, float b) {
		return a / b;
	}, "f32 / f32", cpuInfo, output);
	ipc::writeU64Data(srcBuffer, BufferSize);
	ipc::convU64ToF64(srcBuffer, BufferSize);
	benchmarkOp<double, BufferSize>(durationMeasurer, srcBuffer, buffer, [](double a, double b) {
		return a / b;
	}, "f64 / f64", cpuInfo, output);

	ipc::writeU16Data(srcBuffer, BufferSize);
	benchmarkOp<uint16_t, BufferSize>(durationMeasurer, srcBuffer, buffer, [](uint16_t a, uint16_t b) {
		return a * b;
	}, "u16 * u16", cpuInfo, output);
	ipc::writeU32Data(srcBuffer, BufferSize);
	benchmarkOp<uint32_t, BufferSize>(durationMeasurer, srcBuffer, buffer, [](uint32_t a, uint32_t b) {
		return a * b;
	}, "u32 * u32", cpuInfo, output);
	ipc::writeU64Data(srcBuffer, BufferSize);
	benchmarkOp<uint64_t, BufferSize>(durationMeasurer, srcBuffer, buffer, [](uint64_t a, uint64_t b) {
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

		//benchmark<1 << 6>(measurer, cpuInfoCStr, output);
		benchmark<1 << 7>(measurer, cpuInfoCStr, output);
		benchmark<1 << 8>(measurer, cpuInfoCStr, output);
		benchmark<1 << 9>(measurer, cpuInfoCStr, output);
		benchmark<1 << 10>(measurer, cpuInfoCStr, output);
		//benchmark<1 << 11>(measurer, cpuInfoCStr, output);
		benchmark<1 << 12>(measurer, cpuInfoCStr, output);
		//benchmark<1 << 14>(measurer, cpuInfoCStr, output);
		benchmark<1 << 16>(measurer, cpuInfoCStr, output);
	} catch (const std::exception &e) {
		std::fprintf(stderr, "FATAL ERROR: %s\n", e.what());

		#ifdef _WIN32
		// Do not prompt if running in an msys terminal
		if (std::getenv("MSYSTEM") == nullptr) {
			std::printf("\n");
			std::printf("Press return to exit..\n");
			std::getchar();
		}
		#endif
	}
	return 0;
}