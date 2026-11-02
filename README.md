<div align="center">

**グロートピア** *(Gurotopia)* : Lightweight & Maintained GTPS written in C/C++

[![Makefile](https://github.com/gurotopia/Gurotopia/actions/workflows/make.yml/badge.svg?branch=master)](https://github.com/gurotopia/Gurotopia/actions/workflows/make.yml)
[![Dockerfile](https://github.com/gurotopia/Gurotopia/actions/workflows/docker.yml/badge.svg)](https://github.com/gurotopia/Gurotopia/actions/workflows/docker.yml)
[![Homebrew](https://github.com/gurotopia/Gurotopia/actions/workflows/homebrew.yml/badge.svg?branch=master)](https://github.com/gurotopia/Gurotopia/actions/workflows/homebrew.yml)
[![](https://app.codacy.com/project/badge/Grade/fa8603d6ec2b4485b8e24817ef23ca21)](https://app.codacy.com/gh/gurotopia/Gurotopia/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![](https://dcbadge.limes.pink/api/server/zzWHgzaF7J?style=flat)](https://discord.gg/zzWHgzaF7J)

</div>


---
### <img width="250" height="53" alt="image" src="https://github.com/user-attachments/assets/0a3bee67-ad6c-4e4c-bd0e-aed89d9b5c09" />

### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/archive.svg) 1. Requirements
   - [**MSYS2**](https://www.msys2.org/)
   - [**Visual Studio Code**](https://code.visualstudio.com/): install [C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) for VSCode
   - [**MariaDB Server**](https://mariadb.org/download/?t=mariadb&p=mariadb&r=12.2.2&os=windows&cpu=x86_64&pkg=msi&mirror=accretive)

### 2. Setup MSYS2
   - Locate your MSYS2 folder at `C:\msys64`, open `ucrt64.exe`, and run the following command:
     
     ```bash
     pacman -S --needed mingw-w64-ucrt-x86_64-{gcc,openssl} make
     ```

### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/build.svg) 3. Compile
   - Open the project folder in Visual Studio Code.
   - Press **`Ctrl + Shift + B`** to start the build process.

### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/debug-alt-small.svg) 4. Run
   - After compiling, press **`F5`** to run the server!


---
### <img align="center" width="48" src="https://github.com/user-attachments/assets/b007ac1f-26fe-4fcf-aa3d-57250b4a0f82" /> Linux

### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/archive.svg) 1. Install Dependencies

- Enter command associated with your distribution into the terminal to install nessesary tools.
   <details><summary><img width="22" src="https://github.com/user-attachments/assets/8359ba6e-a9b2-4500-893f-61eaf40e2478" /> Arch</summary>
   <p>
      
   ```bash
   sudo pacman -S base-devel openssl mariadb-libs
   ```
   </p>
   </details> 
   <details><summary><img width="22" src="https://github.com/user-attachments/assets/742f35c4-3e69-450e-8095-9fabe9ecd0d8" /> Debian <img width="18" src="https://github.com/user-attachments/assets/46f0770e-f4ed-480b-851d-c90b05fae52f" /> Ubuntu</summary>
   <p>
      
   ```bash
   sudo apt-get update && sudo apt-get install build-essential libssl-dev libmariadb-dev
   ```
        
   </p>
   </details> 
### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/build.svg) 2. Compile
   - Navigate to the project's root directory in your terminal and run the `make` command:
   
     ```bash
     make -j$(nproc)
     ```
### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/debug-alt-small.svg) 3. Run
   - Execute the compiled binary located in the `main` directory:
   
     ```bash
     ./main.out
     ```


---
### <img align="center" width="48" src="https://github.com/user-attachments/assets/58ce37a9-2e9d-4503-8dd9-9ecf8ce04dcd" /> macOS

> [!WARNING]
> Apple Silicon is not tested yet.

### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/archive.svg) 1. Install Dependencies

- Install [Homebrew](https://brew.sh/) or [MacPorts](https://www.macports.org/install.php), then run the matching command. Mixing is fine: MacPorts GCC with Homebrew OpenSSL and MariaDB uses the Homebrew compile command.
   <details><summary><img align="center" width="24" src="https://github.com/user-attachments/assets/7bb35b38-a46e-4b60-8dcc-9e13c1058480" /> Homebrew</summary>
   <p>
      
   ```bash
   brew install gcc make openssl@3 mariadb-connector-c
   ```
   </p>
   </details>
   <details><summary>MacPorts</summary>
   <p>
      
   ```bash
   sudo port install gcc15 gmake openssl3 mariadb-11.4
   ```
   </p>
   </details>

### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/build.svg) 2. Compile

- Apple's SDK does not include OpenSSL headers. Pass Homebrew or MacPorts paths on the `make` line. A plain `make` fails with `openssl/ssl.h: No such file or directory`. Use `-lenet_macos`; the default `-lenet` is a Linux library.
   <details><summary><img align="center" width="24" src="https://github.com/user-attachments/assets/7bb35b38-a46e-4b60-8dcc-9e13c1058480" /> Homebrew</summary>
   <p>
      
   ```bash
   make -j$(sysctl -n hw.ncpu) \
     includes="-Iinclude -Ibuild/include -I$(brew --prefix openssl@3)/include" \
     libraries="-L$(brew --prefix openssl@3)/lib -L$(brew --prefix mariadb-connector-c)/lib -L./include/enet/lib -L./include/mysql/lib -lssl -lcrypto -lmariadb -lenet_macos"
   ```
   </p>
   </details>
   <details><summary>MacPorts</summary>
   <p>
      
   ```bash
   gmake -j$(sysctl -n hw.ncpu) \
     includes="-Iinclude -Ibuild/include -I/opt/local/libexec/openssl3/include" \
     libraries="-L/opt/local/libexec/openssl3/lib -L/opt/local/lib/mariadb-11.4/mysql -L./include/enet/lib -L./include/mysql/lib -lssl -lcrypto -lmariadb -lenet_macos"
   ```
   </p>
   </details>

### ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/debug-alt-small.svg) 3. Run
   - Execute the compiled binary:
   
     ```bash
     ./main.out
     ```
# ![](https://raw.githubusercontent.com/microsoft/vscode-icons/main/icons/dark/settings.svg) Local Server Configuration

> [!NOTE]
> To connect to your local server, you must modify your system's **hosts** file.
> - **Windows**: `C:\Windows\System32\drivers\etc\hosts`
> - **Linux/macOS**: `/etc/hosts`
> ```
> 127.0.0.1 www.growtopia1.com
> 127.0.0.1 www.growtopia2.com
> ```
