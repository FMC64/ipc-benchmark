#pragma once

#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <cstring>
#include "clock.hpp"

namespace ipc {

static void* mallocAligned(size_t alignment, size_t size) {
	#ifdef _WIN32

	return _aligned_malloc(size, alignment);

	#else

	return aligned_alloc(alignment, size);

	#endif
}

struct Buffer {
	const size_t size;
	void * const data;

	Buffer(size_t size) :
		size(size),
		data(mallocAligned(1 << 16, size))
	{
		if (size > 0 && data == nullptr) {
			std::stringstream ss;
			ss << "ipc::Buffer: Could not allocate " << size << " bytes of data";
			throw std::runtime_error(ss.str());
		}
	}

	Buffer(const Buffer &other) = delete;
	Buffer operator=(const Buffer &other) = delete;

private:
	// data is undefined at return
	void release(void) {
		if (data != nullptr)
			std::free(data);
	}

public:
	Buffer(Buffer &&other) :
		size(other.size),
		data(other.data)
	{
		const_cast<void*&>(other.data) = nullptr;
	}
	Buffer& operator=(Buffer &&other) {
		release();

		const_cast<size_t&>(size) = other.size;
		const_cast<void*&>(data) = other.data;

		const_cast<void*&>(other.data) = nullptr;

		return *this;
	}

	~Buffer(void) {
		release();

		const_cast<void*&>(data) = nullptr;
	}
};

static inline constexpr size_t cycleCountIterationCount = 1 << 16;

static inline void assertBufferSize(const Buffer &buffer, size_t exactSize) {
	if (buffer.size != exactSize) {
		std::stringstream ss;
		ss << "ipc::assertBufferSize: Failed: sizeof(a) ( = " << buffer.size << ") != " << exactSize;
		throw std::runtime_error(ss.str());
	}
}

static inline void assertBufferSizeMultipleOf(const Buffer &a, size_t alignment) {
	if (a.size % alignment != 0) {
		std::stringstream ss;
		ss << "ipc::assertBufferSizeMultipleOf: Failed: sizeof(a) is offset by " << (a.size % alignment) << " bytes, requested alignment on " << alignment << " bytes";
		throw std::runtime_error(ss.str());
	}
}

static inline void assertBufferSizeAtLeast(const Buffer &a, size_t minSize) {
	if (a.size < minSize) {
		std::stringstream ss;
		ss << "ipc::assertBufferSizeAtLeast: Failed: sizeof(a) = " << a.size << " < " << minSize;
		throw std::runtime_error(ss.str());
	}
}

template <size_t WordCount, size_t Off, typename T, typename Op>
static inline void computeCyleCountPerOpPipelinedIteration(T * const words, Op &&op) {
	if constexpr (Off < WordCount) {
		words[Off + 2] = op(words[Off + 0], words[Off + 1]);
		computeCyleCountPerOpPipelinedIteration<WordCount, Off + 4>(words, op);
	}
};

template <size_t OpCount>
static consteval size_t getRepeatCount(void) {
	return (1 << 14) / OpCount;
}

// Op is `T (T a, T b)`
// srcBuffer contains the data to be processed in parallel: packs of [T a, T b, T res, T padding]
template <typename T, size_t BufferSize, typename Op>
Duration computeCyleCountPerOpPipelined(const ipc::DurationMeasurer &durationMeasurer, const Buffer &srcBuffer, Buffer &buffer, Op &&op) {
	assertBufferSizeAtLeast(srcBuffer, BufferSize);
	assertBufferSize(srcBuffer, buffer.size);
	assertBufferSizeMultipleOf(srcBuffer, sizeof(T) * 4);

	constexpr size_t wordCount = BufferSize / sizeof(T);
	constexpr size_t opCount = wordCount / 4;

	static_assert(opCount > 0, "Must have at least a single op to measure");

	constexpr size_t repeatCount = getRepeatCount<opCount>();

	auto sample = [&]() {
		std::memcpy(buffer.data, srcBuffer.data, srcBuffer.size);
		volatile auto words = reinterpret_cast<T * const>(buffer.data);

		return durationMeasurer.measure([&]() {
			constexpr size_t max = wordCount;

			//computeCyleCountPerOpPipelinedIteration<wordCount, 0>(words, std::forward<Op>(op));
			for (size_t r = 0; r < repeatCount; r++) {
				for (size_t i = 0; i < max; i += 4) {
					words[i + 2] = op(words[i + 0], words[i + 1]);
				}
			}
		});
	};

	Duration durations[cycleCountIterationCount];
	for (size_t inst = 0; inst < cycleCountIterationCount; inst++) {
		// Warmup
		durations[inst] = sample();
		// Actual
		durations[inst] = sample();
	}

	auto avgOpDuration = Duration::average(durations) / (opCount * repeatCount);
	return avgOpDuration;
}

// Op is `T (T a, T b)`
// srcBuffer contains the data to be processed serially: packs of [T first, T accumulated0, T accumulated1, ..., T res]
template <typename T, size_t BufferSize, typename Op>
Duration computeCyleCountPerOpSequentially(const ipc::DurationMeasurer &durationMeasurer, const Buffer &srcBuffer, Buffer &buffer, Op &&op) {
	assertBufferSizeAtLeast(srcBuffer, BufferSize);
	assertBufferSize(srcBuffer, buffer.size);
	assertBufferSizeMultipleOf(srcBuffer, sizeof(T) * 4);
	assertBufferSizeAtLeast(srcBuffer, sizeof(T) * 8);

	constexpr size_t wordCount = BufferSize / sizeof(T);
	constexpr size_t opCount = wordCount / 4 - 2;

	static_assert(opCount > 0, "Must have at least a single op to measure");

	constexpr size_t repeatCount = getRepeatCount<opCount>();

	auto sample = [&]() {
		std::memcpy(buffer.data, srcBuffer.data, srcBuffer.size);
		volatile auto words = reinterpret_cast<T * const>(buffer.data);

		return durationMeasurer.measure([&]() {
			constexpr size_t max = wordCount - 4;

			for (size_t r = 0; r < repeatCount; r++) {
				size_t i = 0;
				T acc = words[i];
				i += 4;
				for (; i < max; i += 4) {
					acc = op(acc, words[i]);
					words[i + 2] = words[i + 1];
				}
				words[i] = acc;
			}
		});
	};

	Duration durations[cycleCountIterationCount];
	for (size_t inst = 0; inst < cycleCountIterationCount; inst++) {
		// Warmup
		durations[inst] = sample();
		// Actual
		durations[inst] = sample();
	}

	auto avgOpDuration = Duration::average(durations) / (opCount * repeatCount);
	return avgOpDuration;
}

}