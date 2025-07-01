<div align="center">

**グロートピア** *(Gurotopia)* : Lightweight & Maintained GTPS written in C/C++

[![](https://github.com/GT-api/GT.api/actions/workflows/make.yml/badge.svg)](https://github.com/GT-api/GT.api/actions/workflows/make.yml)
[![MSBuild](https://github.com/gurotopia/Gurotopia/actions/workflows/msbuild.yml/badge.svg)](https://github.com/gurotopia/Gurotopia/actions/workflows/msbuild.yml)
[![Dockerfile](https://github.com/gurotopia/Gurotopia/actions/workflows/docker.yml/badge.svg)](https://github.com/gurotopia/Gurotopia/actions/workflows/docker.yml)
[![](https://app.codacy.com/project/badge/Grade/fa8603d6ec2b4485b8e24817ef23ca21)](https://app.codacy.com/gh/gurotopia/Gurotopia/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![](https://dcbadge.limes.pink/api/server/zzWHgzaF7J?style=flat)](https://discord.gg/zzWHgzaF7J)

</div>

***

### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/checklist.svg) Prerequisites

- **Visual Studio Code**: **https://code.visualstudio.com/** (Recommended Editor)
- **C/C++ extension (VSC)**: https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools
- **SQLite Viewer extension (VSC)**: https://marketplace.visualstudio.com/items?itemName=qwtel.sqlite-viewer

---

### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/platform-windows.svg) Building on Windows

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/archive.svg) 1. Install Dependencies
   - **MSYS2**: **https://www.msys2.org/**
   - Locate your MSYS2 folder (e.g., `C:\msys64`), open `ucrt64.exe`, and run the following command:
     ```bash
     pacman -S --needed mingw-w64-ucrt-x86_64-gcc make
     ```

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/build.svg) 2. Compile
   - Open the project folder in Visual Studio Code.
   - Press **`Ctrl + Shift + B`** to start the build process.

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/debug-alt-small.svg) 3. Run/Debug
   - After compiling, press **`F5`** to run the server with the debugger attached.

---

### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/platform-linux.svg) Building on Linux

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/archive.svg) 1. Install Dependencies
   - Open a terminal and use your distribution's package manager to install the necessary build tools and libraries.

   - **Debian / Ubuntu:**
     ```bash
     sudo apt-get update && sudo apt-get install build-essential libssl-dev libenet-dev libsqlite3-dev
     ```
   - **Fedora / CentOS / RHEL:**
     ```bash
     sudo dnf install gcc-c++ make openssl-devel enet-devel sqlite-devel
     ```
   - **Arch Linux:**
     ```bash
     sudo pacman -S base-devel openssl enet sqlite
     ```

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/build.svg) 2. Compile
   - Navigate to the project's root directory in your terminal and run the `make` command:
     ```bash
     make
     ```

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/debug-alt-small.svg) 3. Run
   - Execute the compiled binary located in the `build` directory:
     ```bash
     ./build/main.out
     ```

---

### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/settings.svg) Local Server Configuration

> [!NOTE]
> To connect to your local server, you must modify your system's **hosts** file.
> - **Windows**: `C:\Windows\System32\drivers\etc\hosts`
> - **Linux/macOS**: `/etc/hosts`
>
> Add the following lines to the end of the file:
> ```
> 127.0.0.1 [www.growtopia1.com](https://www.growtopia1.com)
> 127.0.0.1 [www.growtopia2.com](https://www.growtopia2.com)
> ```