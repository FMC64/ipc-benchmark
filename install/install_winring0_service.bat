xcopy .\WinRing0x64.sys C:\Windows\SysWOW64\drivers
sc create WinRing0 binPath= C:\Windows\SysWOW64\drivers\WinRing0x64.sys type= kernel
sc config WinRing0 start= auto
sc start WinRing0