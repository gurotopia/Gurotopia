@echo off
echo Gurotopia installer 2026-1-30 sha-132b1ac99f164b499de2b84f6cb9dc14a7fa7534

if not exist "%USERPROFILE%\AppData\Local\Programs\Microsoft VS Code\Code.exe" (
	powershell -Command "Start-BitsTransfer -Source 'https://code.visualstudio.com/sha/download?build=stable&os=win32-x64-user' -Destination 'vscode.exe'"
	start /wait vscode.exe /silent /mergetasks=!runcode
	del vscode.exe
)

where code >nul 2>nul && (
	code --list-extensions | findstr /I "ms-vscode.cpptools" >nul || (
		code --install-extension ms-vscode.cpptools
	)
)

if not exist "C:\msys64\usr\bin\bash.exe" (
	powershell -Command "Start-BitsTransfer -Source 'https://github.com/msys2/msys2-installer/releases/latest/download/msys2-x86_64-latest.exe' -Destination 'msys2.exe'"
	start /wait msys2.exe --confirm-command --accept-messages --no-start
	del msys2.exe
)

C:\msys64\usr\bin\bash.exe -lc "pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-{gcc,openssl,sqlite} make"

:: open Gurotopia in vscode @todo automatically build Gurotopia.
code . main.cpp
