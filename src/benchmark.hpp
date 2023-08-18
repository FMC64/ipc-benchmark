#pragma once

#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include "clock.hpp"

namespace ipc {

struct Buffer {
	size_t size;
	void *data;

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
		other.data = nullptr;
	}
	Buffer& operator=(Buffer &&other) {
		release();

		size = other.size;
		data = other.data;

		other.data = nullptr;

		return *this;
	}

	~Buffer(void) {
		release();

		data = nullptr;
	}
};

}