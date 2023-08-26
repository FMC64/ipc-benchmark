# How to install

- First, from this directory start a `cmd.exe` or Powershell prompt as Administrator and run `enable_test_services.bat`
	- This will allow self-signed services such as `WinRing0x64.sys` to be installed
- Reboot your Windows PC
- <ins>OPTIONAL</ins>: If you prefer to, you can build `WinRing0x64.sys` yourself: refer to `README.md` and follow the steps under `Visual Studio Community 2022 is necessary, build both 'WinRing0Dll' and 'WinRing0Sys' in Release.`
	- Overwrite the `WinRing0x64.sys` provided here with your own located at `https://github.com/GermanAizek/WinRing0\x64\Release`
- From this directory start a `cmd.exe` or Powershell prompt as Administrator and run `install_winring0_service.bat`
	- At this point, the `WinRing0x64.sys` service is installed and running
- You can now run `ipc-benchmark.exe` (needs Administrator prompt as well)

# How to uninstall

- From this directory start a `cmd.exe` or Powershell prompt as Administrator and run `uninstall_winring0_service.bat`
	- This will stop `WinRing0x64.sys`, delete the service and remove it from system files
- From the same Administrator prompt, run `disable_test_services.bat`
- Reboot your Windows PC to get back at the same state before installation