# vcpkg installation on Windows

This document describes the process of installing the `vcpkg` package manager and its integration with the system and Visual Studio.

## 1. Prerequisites
* **Git** installed ([git-scm.com](https://git-scm.com/)).
* **Visual Studio 2022** (or 2019) installed with the "Desktop development with C++" workload selected.

## 2. Installing vcpkg

1. Open a terminal (PowerShell or Command Prompt) in the folder where you want to keep `vcpkg` (e.g., `C:\dev\vcpkg`).
2. Clone the repository, booststrap and integrate with Windows/Visual Studio:
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg.exe integrate install
   ```
