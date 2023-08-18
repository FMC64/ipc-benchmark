#pragma once

#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <cstring>
#include "clock.hpp"

namespace ipc {

struct Buffer {
	const size_t size;
	void * const data;

	Buffer(size_t size) :
		size(size),
		data(std::malloc(size))
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

// Op is `T (T a, T b)`
// srcBuffer contains the data to be processed in parallel: packs of [T a, T b, T res, T padding]
template <typename T, size_t BufferSize, typename Op>
Duration computeCyleCountPerOpPipelined(const ipc::DurationMeasurer &durationMeasurer, const Buffer &srcBuffer, Buffer &buffer, Op &&op) {
	assertBufferSize(srcBuffer, BufferSize);
	assertBufferSize(srcBuffer, buffer.size);
	assertBufferSizeMultipleOf(srcBuffer, sizeof(T) * 4);

	constexpr size_t wordCount = BufferSize / sizeof(T);
	constexpr size_t opCount = wordCount / 4;

	auto sample = [&]() {
		std::memcpy(buffer.data, srcBuffer.data, srcBuffer.size);
		auto words = reinterpret_cast<T * const>(buffer.data);

		return durationMeasurer.measure([&]() {
			size_t off = 0;
			for (size_t i = 0; i < opCount; i++) {
				words[off + 2] = op(words[off + 0], words[off + 1]);
				off += 4;
			}
		});
	};

	Duration durations[cycleCountIterationCount];
	for (size_t inst = 0; inst < cycleCountIterationCount; inst++) {
		for (size_t i = 0; i < 4; i++)
			durations[inst] = sample();
		durations[inst] = sample();
	}

	auto avgOpDuration = Duration::average(durations) / opCount;
	return avgOpDuration;
}

// Op is `T (T a, T b)`
// srcBuffer contains the data to be processed serially: packs of [T first, T accumulated0, T accumulated1, ..., T res]
template <typename T, size_t BufferSize, typename Op>
Duration computeCyleCountPerOpSequentially(const ipc::DurationMeasurer &durationMeasurer, const Buffer &srcBuffer, Buffer &buffer, Op &&op) {
	assertBufferSize(srcBuffer, BufferSize);
	assertBufferSize(srcBuffer, buffer.size);
	assertBufferSizeAtLeast(srcBuffer, sizeof(T) * 2);

	constexpr size_t wordCount = BufferSize / sizeof(T);
	constexpr size_t opCount = wordCount - 2;

	auto sample = [&]() {
		std::memcpy(buffer.data, srcBuffer.data, srcBuffer.size);
		auto words = reinterpret_cast<T * const>(buffer.data);

		return durationMeasurer.measure([&]() {
			size_t off = 1;
			T acc = words[0];
			for (size_t i = 0; i < opCount; i++) {
				acc = op(acc, words[off]);
				off++;
			}
			words[wordCount - 1] = acc;
		});
	};

	Duration durations[cycleCountIterationCount];
	for (size_t inst = 0; inst < cycleCountIterationCount; inst++) {
		for (size_t i = 0; i < 4; i++)
			durations[inst] = sample();
		durations[inst] = sample();
	}

	auto avgOpDuration = Duration::average(durations) / opCount;
	return avgOpDuration;
}

}