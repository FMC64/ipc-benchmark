#pragma once

#include "benchmark.hpp"

namespace ipc {

void writeU16Data(ipc::Buffer &dst, size_t bufferSize);
void writeU32Data(ipc::Buffer &dst, size_t bufferSize);
void writeU64Data(ipc::Buffer &dst, size_t bufferSize);

void convU32ToF32(ipc::Buffer &dst, size_t bufferSize);
void convU64ToF64(ipc::Buffer &dst, size_t bufferSize);

}