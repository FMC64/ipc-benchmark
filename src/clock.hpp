#pragma once

#ifdef _WIN32

#include <intrin.h>
#include <windows.h>

#else

#include <x86intrin.h>
#include <unistd.h>
#include <string.h>

#endif

#include <Tchar.h>
extern "C" {
#include <WinRing0/OlsApi.h>
}

#include <cstdint>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <sstream>
#include <cstdio>
#include <array>
#include <cstring>
#include <fstream>
#include <vector>
#include <functional>

namespace ipc {

#ifdef _WIN32

// Largely adapted from https://stackoverflow.com/a/17387176
// Inefficient but who cares, everything is already going down if that gets called
static inline std::string winGetLastError(void) {
	//Get the error message ID, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0) {
		return std::string(); //No error message has been recorded
	}

	LPSTR messageBuffer = nullptr;

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL
	);

	//Copy the error message into a std::string.
	std::string message(messageBuffer, size);

	//Free the Win32's string's buffer.
	LocalFree(messageBuffer);
	return message;
}

#endif

static inline std::string getCPUInfo(void)
{
	#ifdef _WIN32

	// From https://stackoverflow.com/a/64422512

        // 4 is essentially hardcoded due to the __cpuid function requirements.
        // NOTE: Results are limited to whatever the sizeof(int) * 4 is...
        std::array<int, 4> integerBuffer = {};
        constexpr size_t sizeofIntegerBuffer = sizeof(int) * integerBuffer.size();

        std::array<char, 64> charBuffer = {};

        // The information you wanna query __cpuid for.
        // https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=vs-2019
        constexpr std::array<uint32_t, 3> functionIds = {
		// Manufacturer
		//  EX: "Intel(R) Core(TM"
		0x8000'0002,
		// Model
		//  EX: ") i7-8700K CPU @"
		0x8000'0003,
		// Clockspeed
		//  EX: " 3.70GHz"
		0x8000'0004
        };

        std::string cpu;

        for (int id : functionIds)
        {
		// Get the data for the current ID.
		__cpuid(integerBuffer.data(), id);

		// Copy the raw data from the integer buffer into the character buffer
		std::memcpy(charBuffer.data(), integerBuffer.data(), sizeofIntegerBuffer);

		// Copy that data into a std::string
		cpu += std::string(charBuffer.data());
        }

        return cpu;

	#else

	std::ifstream input("/proc/cpuinfo", std::ios::in);
	if (!input.good())
		throw std::runtime_error("ipc::getCPUInfo: Could not open /proc/cpuinfo");

	auto split = [](const std::string &str, char delim) {
		std::stringstream tokenSs(str);
		std::vector<std::string> tokens;
		std::string token;
		while (std::getline(tokenSs, token, delim))
			tokens.emplace_back(token);
		return tokens;
	};

	auto join = [](const std::vector<std::string> &strs) {
		std::string res;
		for (auto &str : strs)
			res += str;
		return res;
	};

	auto trim = [](const std::string &str) {
		std::string res;
		for (auto c : str) {
			if (c == ' ' || c == '\n' || c == '\t' || c == '\r')
				continue;
			res.push_back(c);
		}
		return res;
	};

	std::string line;
	while (std::getline(input, line)) {
		auto sections = split(line, ':');
		if (sections.size() < 2)
			continue;

		auto id = split(sections[0], ' ');
		if (id.size() < 2)
			continue;
		if (trim(id[0]) == "model" && trim(id[1]) == "name") {
			std::vector<std::string> modelName;
			for (size_t i = 1; i < sections.size(); i++)
				modelName.emplace_back(sections[i]);
			modelName[0] = modelName[0].substr(1);
			return join(modelName);
		}
	}

	throw std::runtime_error("ipc::getCPUInfo: /proc/cpuinfo did not contain a single entry with 'model name'");

	#endif
}

static inline void setRealtime(void) {
#ifdef _WIN32

	auto res = SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	if (res == 0) {
		std::stringstream ss;
		ss << "ipc::setRealtime(_WIN32): SetPriorityClass: " << winGetLastError();
		throw std::runtime_error(ss.str());
	}

	auto priority = GetPriorityClass(GetCurrentProcess());
	if (priority == 0) {
		std::stringstream ss;
		ss << "ipc::setRealtime(_WIN32): GetPriorityClass: " << winGetLastError();
		throw std::runtime_error(ss.str());
	}
	if (priority != REALTIME_PRIORITY_CLASS) {
		std::stringstream ss;
		ss << "ipc::setRealtime(_WIN32): " << "System call did succeed but priority is still lower than required (have 0x" << std::hex << priority << "). Try running the program with administrator priviledges to have the realtime mode go through.";
		throw std::runtime_error(ss.str());
	}

	std::printf("ipc::setRealtime: Certified now running in realtime mode!\n");

#else

	std::printf("ipc::setRealtime: Disclaimer: it is advised not to run this in WSL unless WSL itself is running as realtime (which sounds like a crazy & horribly scary idea). Just pick up the Windows binary in this case.\n");

	auto res = nice(-20);
	if (res == -1) {
		std::stringstream ss;
		ss << "ipc::setRealtime(unistd.h): " << strerror(errno);
		throw std::runtime_error(ss.str());
	}

	std::printf("ipc::setRealtime: Nice value for the process is now %d\n", res);

#endif
}

static inline size_t getTscTimestamp(void) {
	return __rdtsc();
}

struct Duration {
	double lengthCycles;
	double lengthSeconds;

	Duration operator/(size_t n) const {
		return Duration{
			.lengthCycles = lengthCycles / static_cast<double>(n),
			.lengthSeconds = lengthSeconds / static_cast<double>(n)
		};
	}

	template <size_t N>
	static Duration average(const Duration (&samples)[N]) {
		Duration res = {
			.lengthCycles = 0.0,
			.lengthSeconds = 0.0
		};

		for (size_t i = 0; i < N; i++) {
			auto cur = samples[i];
			res.lengthCycles += cur.lengthCycles;
			res.lengthSeconds += cur.lengthSeconds;
		}

		return res / N;
	}

	double inferredFrequency(void) const {
		return lengthCycles / lengthSeconds;
	}

	double inferredFrequencyMHz(void) const {
		return inferredFrequency() / 1.0e6;
	}
};

class DurationMeasurer
{
	std::function<double (void)> m_getFrequency;
	// How much time does sampling take
	Duration m_overhead;

	// How many dummy samplings to average to calibrate
	// Warning: do not set this too high, as std::this_thread::sleep_for may just skip
	static inline constexpr size_t calibrationIterationCount = 1 << 8;
	static inline constexpr double calibrationLengthSeconds = 4.0;

	static inline void cpuID(DWORD index, DWORD *eax, DWORD *ebx, DWORD *ecx, DWORD *edx) {
		DWORD beax, bebx, becx, bedx;
		if (!Cpuid(index, eax != nullptr ? eax :& beax, ebx != nullptr ? ebx : &bebx, ecx != nullptr ? ecx : &becx, edx != nullptr ? edx : &bedx)) {
			std::stringstream ss;
			ss << "ipc::DurationMeasurer::cpuID: Could not read index 0x" << std::hex << index;
			throw std::runtime_error(ss.str());
		}
	}

	static inline uint64_t readMSR(DWORD index) {
		DWORD low, high;
		if (!Rdmsr(index, &low, &high)) {
			std::stringstream ss;
			ss << "ipc::DurationMeasurer::readMSR: Could not read index 0x" << std::hex << index;
			throw std::runtime_error(ss.str());
		}
		return (static_cast<uint64_t>(high) << 32) | static_cast<uint64_t>(low);
	}

	// This funcion is largely ported from https://github.com/openhardwaremonitor/openhardwaremonitor
	static inline std::function<double (void)> getFrequencyGetter(void) {
		{
			if (!InitializeOls())
				throw std::runtime_error("ipc::DurationMeasurer::InitOpenLibSys: Failure. Is the WinRing0 service installed & running?");
		}
		if (!IsMsr())
			throw std::runtime_error("ipc::DurationMeasurer::IsMsr: Failure. Cannot read MSRs.");
		std::printf("WinRing0: Initialized!\n");

		std::string vendor;
		size_t family;
		std::vector<DWORD> payload;
		{
			DWORD lastCPUIDIndex = 0, vendorReg[3];
			cpuID(0, &lastCPUIDIndex, &vendorReg[0], &vendorReg[2], &vendorReg[1]);
			if (lastCPUIDIndex == 0)
				throw std::runtime_error("ipc::DurationMeasurer:getFrequencyGetter: CPUID index 0 failed: lastCPUIDIndex is zero");
			size_t CPUIDCount = lastCPUIDIndex + 1;
			size_t payloadSize = CPUIDCount * 4;
			payload.resize(payloadSize);
			for (size_t i = 0; i < CPUIDCount; i++) {
				auto offd = &payload[i * 4];
				cpuID(i, &offd[0], &offd[1], &offd[2], &offd[3]);
			}

			auto appendRegToString = [](DWORD reg, std::string &dst) {
				static_assert(sizeof(DWORD) == 4, "DWORD size must be 4");
				for (size_t i = 0; i < 4; i++)
					dst.push_back((reg >> (i * 8)) & 0xFF);
			};
			for (size_t i = 0; i < 3; i++)
				appendRegToString(vendorReg[i], vendor);
			auto familyCode = payload[4 * 1];
			family = ((familyCode & 0x0FF00000) >> 20) + ((familyCode & 0x0F00) >> 8);

			std::printf("Vendor: %s, family = %zx\n", vendor.c_str(), family);
		}

		auto getTscFreq = []() {
			std::printf("Estimating TSC frequency..\n");
			auto bef = std::chrono::high_resolution_clock::now();
			auto tscBef = __rdtsc();
			std::this_thread::sleep_for(std::chrono::seconds(4));
			auto aft = std::chrono::high_resolution_clock::now();
			auto tscAft = __rdtsc();

			auto res = static_cast<double>(tscAft - tscBef) / static_cast<std::chrono::duration<double>>(aft - bef).count();
			std::printf("TSC at %g MHz\n", res / 1.0e6);

			return res;
		};

		if (vendor == "GenuineIntel") {
			auto tscFreq = getTscFreq();
			return [tscFreq]() {
				return tscFreq;
			};
		} else if (vendor == "AuthenticAMD") {
			if (family == 0x17 || family == 0x19) {
				auto getTscInvFactor = []() {
					auto a = readMSR(0xC0010064);
					auto cpuDfsId = (a >> 8) & 0x3f;
					auto cpuFid = a & 0xff;
					return 2.0 * static_cast<double>(cpuFid) / static_cast<double>(cpuDfsId);
				};

				double busClock = getTscFreq() / getTscInvFactor();
				return [busClock]() {
					auto a = readMSR(0xc0010293);
					auto cpuDfsId = (a >> 8) & 0x3f;
					auto cpuFid = a & 0xff;
					auto multiplier = 2.0 * static_cast<double>(cpuFid) / static_cast<double>(cpuDfsId);
					return busClock * multiplier;
				};
			} else {
				std::stringstream ss;
				ss << "ipc::DurationMeasurer:getFrequencyGetter: Unsupported family 0x" << std::hex << family << " for vendor '" << vendor << "'";
				throw std::runtime_error(ss.str());
			}
		} else {
			std::stringstream ss;
			ss << "ipc::DurationMeasurer:getFrequencyGetter: Unknown vendor '" << vendor << "'";
			throw std::runtime_error(ss.str());
		}
	}

public:
	DurationMeasurer(void) :
		m_getFrequency(getFrequencyGetter()),
		m_overhead({
			.lengthCycles = 0.0,
			.lengthSeconds = 0.0
		})
	{
		for (size_t i = 0; i < 16; i++) {
			std::printf("Current frequency: %g MHz\n", m_getFrequency() / 1.0e6);
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
	~DurationMeasurer(void) {
		DeinitializeOls();
	}

	// Only informative, do not use these timings yourself, measure already does the compensation by default
	Duration getOverhead(void) const {
		return m_overhead;
	}

	// Fn is `void (void)`
	template <typename Fn>
	Duration measure(Fn &&fn, bool compensate = true) const {
		auto beginChrono = std::chrono::high_resolution_clock::now();
		volatile auto begin = getTscTimestamp();
		fn();
		volatile auto end = getTscTimestamp();
		auto endChrono = std::chrono::high_resolution_clock::now();

		auto res = Duration{
			.lengthCycles = static_cast<double>(end - begin),
			.lengthSeconds = std::chrono::duration<double>(endChrono - beginChrono).count()
		};
		if (compensate) {
			if (res.lengthCycles > m_overhead.lengthCycles)
				res.lengthCycles -= m_overhead.lengthCycles;
			else
				res.lengthCycles = 0.0;

			if (res.lengthSeconds > m_overhead.lengthSeconds)
				res.lengthSeconds -= m_overhead.lengthSeconds;
			else
				res.lengthSeconds = 0.0;
		}
		return res;
	}

private:
	Duration computeCalibration(void) const {

		std::printf("ipc::DurationMeasurer::computeCalibration: Calibrating, this should take around %g seconds..\n", calibrationLengthSeconds);

		constexpr double lengthPerIteration = calibrationLengthSeconds / static_cast<double>(calibrationIterationCount);


		Duration durations[calibrationIterationCount];

		for (size_t i = 0; i < calibrationIterationCount; i++) {
			std::this_thread::sleep_for(std::chrono::duration<double>(lengthPerIteration));
			// Warmup
			for (size_t i = 0; i < 64; i++) {
				durations[i] = measure([](){}, false);
			}
			// Actual data
			durations[i] = measure([](){}, false);
		}

		std::printf("ipc::DurationMeasurer::computeCalibration: Calibration done.\n");

		return Duration::average(durations);
	}

public:
	void calibrate(void) {
		m_overhead = computeCalibration();
	}
};

}