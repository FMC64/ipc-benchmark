# IPC Benchmark

Humble project to measure Intructions Per Cycle (IPC) of a CPU.  

## Building

### Dependencies

- A C++ runtime should be needed
- Under Windows, the Windows SDK is required along with building the actual application with MinGW-w64
	- You need to build `WinRing0`: https://github.com/GermanAizek/WinRing0
		- Visual Studio Community 2022 is necessary, build both `WinRing0Dll` and `WinRing0Sys` in Release.
			- You will need the very latest Windows SDK along with the WDK: `https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk`
			- From Visual Studio Installer, also install the latest Spectre-mitigated libraries for your build system (from Individual components)
			- You should be able to build both projects in Release now. Do not worry about .NET projects, we are not using those.
		- Make the headers available to MinGW-w64: `mkdir /mingw64/include/WinRing0 && cp ./WinRing0Dll/*.h /mingw64/include/WinRing0`
		- Make `-lWinRing0x64` succeed: `cp WinRing0x64.dll /mingw64/lib/`
		- Then copy `WinRing0x64.dll` and `WinRing0x64.sys` to the root of this repository
		- Follow `INSTALL.md` to install the `WinRing0x64.sys` service
			- `ipc-benchmark.exe` won't start unless this service is running

### The actual application

- You will need a C++ compiler and GNU Make on a Unix-like system
	- ~~Linux has been tested (Debian)~~ Linux needs porting of the performance counters
	- Windows has been tested, using MinGW-w64

- Run
```
make
```

- `ipc-benchmark[.exe]` should appear at the root of the repository