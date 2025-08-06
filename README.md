<div align="center">

**グロートピア** *(Gurotopia)* : Lightweight & Maintained GTPS written in C/C++

[![](https://github.com/GT-api/GT.api/actions/workflows/make.yml/badge.svg)](https://github.com/GT-api/GT.api/actions/workflows/make.yml)
[![MSBuild](https://github.com/gurotopia/Gurotopia/actions/workflows/msbuild.yml/badge.svg)](https://github.com/gurotopia/Gurotopia/actions/workflows/msbuild.yml)
[![Dockerfile](https://github.com/gurotopia/Gurotopia/actions/workflows/docker.yml/badge.svg)](https://github.com/gurotopia/Gurotopia/actions/workflows/docker.yml)
[![](https://app.codacy.com/project/badge/Grade/fa8603d6ec2b4485b8e24817ef23ca21)](https://app.codacy.com/gh/gurotopia/Gurotopia/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![](https://dcbadge.limes.pink/api/server/zzWHgzaF7J?style=flat)](https://discord.gg/zzWHgzaF7J)

</div>

***

# <img width="190" height="50" alt="Windows_logo_and_wordmark_-_2021 svg" src="https://github.com/user-attachments/assets/1385f762-2c56-465a-aa3b-901a431552bb" />

You can build Gurotopia on Windows using either **MSYS2** (recommended) or **MSVC**. Choose the method that best suits your development environment.

### <img width="18" height="18" src="https://github.com/user-attachments/assets/12d92d94-80c7-4330-b4f3-1c8e67ee83a0" /> <img width="18" height="18" src="https://github.com/user-attachments/assets/1f2496c7-bc06-4c64-b2ae-6da04c266484" /> Method 1: MSYS2

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/archive.svg) 1. Install Dependencies
   - download [**MSYS2**](https://www.msys2.org/) and [**Visual Studio Code**](https://code.visualstudio.com/): install C/C++ extension for VSC (**Required**)
     
   - Locate your MSYS2 folder (e.g., `C:\msys64`), open `ucrt64.exe`, and run the following command:
     ```bash
     pacman -S --needed mingw-w64-ucrt-x86_64-gcc make
     ```

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/build.svg) 2. Compile
   - Open the project folder in Visual Studio Code.
   - Press **`Ctrl + Shift + B`** to start the build process.

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/debug-alt-small.svg) 3. Run/Debug
   - After compiling, press **`F5`** to run the server with the debugger attached.

### <img width="20" height="20" alt="2022_logo" src="https://github.com/user-attachments/assets/c69cfaad-f31c-4cad-a93d-8c09973ab3f9" /> Method 2: MSVC (Visual Studio)
#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/archive.svg) 1. Install Dependencies

- [**Visual Studio 2022**](https://visualstudio.microsoft.com/vs/): Install with C++ development tools

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/build.svg) 2. Compile

- Open the `msvc.sln` file using **Visual Studio 2022**.
- Build the solution by pressing **`Ctrl + Shift + B`** or selecting **Build > Build Solution** from the menu.

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/debug-alt-small.svg) 3. Run/Debug

- After compiling, press **`F5`** to run the server with the debugger attached.

# <img src="https://github.com/user-attachments/assets/fecde323-04c5-4b82-a08d-badcb184be6a" width="30" /> Linux

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/archive.svg) 1. Install Dependencies

- enter command associated with your distribution into the terminal to install nessesary tools.
   <details><summary><img width="18" height="18" src="https://github.com/user-attachments/assets/8359ba6e-a9b2-4500-893f-61eaf40e2478" /> Arch</summary>
   <p>
      
   ```bash
   sudo pacman -S base-devel
   ```
   </p>
   </details> 
   <details><summary><img width="18" height="18" src="https://github.com/user-attachments/assets/742f35c4-3e69-450e-8095-9fabe9ecd0d8" /> Debian <img width="18" height="18" src="https://github.com/user-attachments/assets/46f0770e-f4ed-480b-851d-c90b05fae52f" /> Ubuntu</summary>
   <p>
      
   ```bash
   sudo apt-get update && sudo apt-get install build-essential
   ```
        
   </p>
   </details> 

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/build.svg) 2. Compile
   - Navigate to the project's root directory in your terminal and run the `make` command:
     ```bash
     make -j$(nproc)
     ```

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/debug-alt-small.svg) 3. Run
   - Execute the compiled binary located in the `main` directory:
     ```bash
     ./main.out
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
> 127.0.0.1 www.growtopia1.com
> 127.0.0.1 www.growtopia2.com
> ```
