<div align="center">

**グロートピア** *(Gurotopia)* : Lightweight & Maintained GTPS written in C/C++

[![](https://github.com/GT-api/GT.api/actions/workflows/make.yml/badge.svg)](https://github.com/GT-api/GT.api/actions/workflows/make.yml)
[![Dockerfile](https://github.com/gurotopia/Gurotopia/actions/workflows/docker.yml/badge.svg)](https://github.com/gurotopia/Gurotopia/actions/workflows/docker.yml)
[![](https://app.codacy.com/project/badge/Grade/fa8603d6ec2b4485b8e24817ef23ca21)](https://app.codacy.com/gh/gurotopia/Gurotopia/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![](https://dcbadge.limes.pink/api/server/zzWHgzaF7J?style=flat)](https://discord.gg/zzWHgzaF7J)

</div>

***

# <img width="190" height="50" alt="Windows_logo_and_wordmark_-_2021 svg" src="https://github.com/user-attachments/assets/1385f762-2c56-465a-aa3b-901a431552bb" />

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/archive.svg) 1. Install Dependencies
   - download [**MSYS2**](https://www.msys2.org/) and [**Visual Studio Code**](https://code.visualstudio.com/): install [C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) for VSC (**Required**)
     
   - Locate your MSYS2 folder (e.g., `C:\msys64`), open `ucrt64.exe`, and run the following command:
     ```bash
     pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-sqlite make
     ```

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/build.svg) 2. Compile
   - Open the project folder in Visual Studio Code.
   - Press **`Ctrl + Shift + B`** to start the build process.

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/debug-alt-small.svg) 3. Run/Debug
   - After compiling, press **`F5`** to run the server with the debugger attached.

# <img src="https://github.com/user-attachments/assets/fecde323-04c5-4b82-a08d-badcb184be6a" width="30" /> Linux

#### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/archive.svg) 1. Install Dependencies

- enter command associated with your distribution into the terminal to install nessesary tools.
   <details><summary><img width="18" height="18" src="https://github.com/user-attachments/assets/8359ba6e-a9b2-4500-893f-61eaf40e2478" /> Arch</summary>
   <p>
      
   ```bash
   sudo pacman -S base-devel openssl sqlite
   ```
   </p>
   </details> 
   <details><summary><img width="18" height="18" src="https://github.com/user-attachments/assets/742f35c4-3e69-450e-8095-9fabe9ecd0d8" /> Debian <img width="18" height="18" src="https://github.com/user-attachments/assets/46f0770e-f4ed-480b-851d-c90b05fae52f" /> Ubuntu</summary>
   <p>
      
   ```bash
   sudo apt-get update && sudo apt-get install build-essential libssl-dev openssl sqlite3
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
