@echo off
echo Gurotopia installer 2025-10-23 sha-153a00624228785ded334518fa68c9a469ed2789

if not exist "%USERPROFILE%\AppData\Local\Programs\Microsoft VS Code\Code.exe" (
	powershell -Command "Start-BitsTransfer -Source 'https://code.visualstudio.com/sha/download?build=stable&os=win32-x64-user' -Destination 'installer.exe'"
	start /wait installer.exe /silent /mergetasks=!runcode
	del installer.exe
)

set "PATH=%PATH%;%USERPROFILE%\AppData\Local\Programs\Microsoft VS Code\bin"

where code >nul 2>nul && (
	code --list-extensions | findstr /I "ms-vscode.cpptools" >nul || (
		code --install-extension ms-vscode.cpptools
	)
)

if not exist "C:\msys64\usr\bin\bash.exe" (
	powershell -Command "Start-BitsTransfer -Source 'https://github.com/msys2/msys2-installer/releases/latest/download/msys2-x86_64-latest.exe' -Destination installer.exe"
	start /wait installer.exe --confirm-command --accept-messages --no-start
	del installer.exe
)

C:\msys64\usr\bin\bash.exe -lc "pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-{gcc,openssl,sqlite} make"

:: open Gurotopia in vscode @todo automatically build Gurotopia.
code . main.cpp
