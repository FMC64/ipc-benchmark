#include "data.hpp"

namespace ipc {

void writeU16Data(ipc::Buffer &dst, size_t bufferSize) {
	auto u16Data = reinterpret_cast<uint16_t*>(dst.data);
	auto u16Count = bufferSize / sizeof(uint16_t);
	for (size_t i = 0; i < u16Count; i++)
		u16Data[i] = 1 % 4 == 0 ? (1 << 12) - i + 7 : i + 487;
}

void writeU32Data(ipc::Buffer &dst, size_t bufferSize) {
	auto u32Data = reinterpret_cast<uint32_t*>(dst.data);
	auto u32Count = bufferSize / sizeof(uint32_t);
	for (size_t i = 0; i < u32Count; i++)
		u32Data[i] = 1 % 4 == 0 ? (1 << 20) - i + 7 : i + 16487;
}

void writeU64Data(ipc::Buffer &dst, size_t bufferSize) {
	auto u64Data = reinterpret_cast<uint64_t*>(dst.data);
	auto u64Count = bufferSize / sizeof(uint64_t);
	for (size_t i = 0; i < u64Count; i++)
		u64Data[i] = 1 % 4 == 0 ? (static_cast<size_t>(1) << 42) - i + 7 : i + 16487;
}

void convU32ToF32(ipc::Buffer &dst, size_t bufferSize) {
	auto u32Data = reinterpret_cast<uint32_t*>(dst.data);
	auto f32Data = reinterpret_cast<float*>(dst.data);
	auto u32Count = bufferSize / sizeof(uint32_t);
	for (size_t i = 0; i < u32Count; i++) {
		auto val = u32Data[i];
		f32Data[i] = static_cast<float>(val);
	}
}

void convU64ToF64(ipc::Buffer &dst, size_t bufferSize) {
	auto u64Data = reinterpret_cast<uint64_t*>(dst.data);
	auto f64Data = reinterpret_cast<double*>(dst.data);
	auto u64Count = bufferSize / sizeof(uint64_t);
	for (size_t i = 0; i < u64Count; i++) {
		auto val = u64Data[i];
		f64Data[i] = static_cast<double>(val);
	}
}

}