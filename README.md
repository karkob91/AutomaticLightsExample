# AutomaticLightsProject - User Guide

## Table of Contents
1. [Introduction](#introduction)
2. [Prerequisites](#prerequisites)
3. [Setup](#setup)
4. [Scripts Overview](#scripts-overview)
5. [How to Use](#how-to-use)
6. [Troubleshooting](#troubleshooting)
7. [Contact](#contact)

---

## Introduction
Welcome to the **AutomaticLightsProject**! This guide provides instructions for setting up and using the verification and build scripts on both Windows and Linux platforms. The project includes Python, `.bat`, and `.sh` scripts to streamline verification and building processes for Dezyne models.

---

## Prerequisites

### General Requirements
- **Python**: Version 3.8 or later.
- **CMake**: Version 3.20 or later.
- **Dezyne Tool**:
  - Download from [Verum.com](https://www.verum.com/download) and extract it to a suitable location.
  - It is recommended to use:
    - The home directory on Linux (e.g., `~/dezyne-2.18.3`).
    - A path like `C:/dezyne-2.18.3` on Windows.

### Compiler
- **Windows**:
  - **MinGW**: Install via [MinGW-w64](https://www.mingw-w64.org/).
  - **MSVC**: Install via Visual Studio with the "Desktop Development with C++" workload.
- **Linux**:
  - **GCC**: Install via your package manager (e.g., `sudo apt install build-essential` on Ubuntu).

---

## Setup

1. **Clone or download the repository**:
    ```bash
    git clone https://github.com/yourusername/AutomaticLightsProject.git
    cd AutomaticLightsProject
    ```

2. **Configure the `config.json` file**:
    The project uses a JSON configuration file to specify key paths required for verification and building processes. Below is an example configuration:

    ```json
    {
      "projectName": "AutomaticLightsProject",
      "dznPath": "<path_to_dezyne>",
      "dezyneRuntimePath": "<path_to_dezyne_runtime>"
    }
    ```

    - **On Windows**:
        - Use double backslashes (`\\`) as path separators to avoid escape sequence issues.
        - Example:
          ```json
          {
            "projectName": "AutomaticLightsProject",
            "dznPath": "C:\\path\\to\\dezyne",
            "dezyneRuntimePath": "C:\\path\\to\\dezyne\\runtime\\c++"
          }
          ```

    - **On Linux**:
        - Use forward slashes (`/`) as path separators.
        - Example:
          ```json
          {
            "projectName": "AutomaticLightsProject",
            "dznPath": "/path/to/dezyne",
            "dezyneRuntimePath": "/path/to/dezyne/runtime/c++"
          }
          ```

---

## Scripts Overview

### Verification Scripts
- **`verify_models.py`**:
  - A Python script for verifying `.dzn` models.
  - Compatible with both Windows and Linux.
- **`verify_models.bat`**:
  - A batch script for verifying `.dzn` models on Windows.
- **`verify_models.sh`**:
  - A shell script for verifying `.dzn` models on Linux.

### Build Scripts
- **`build_script.py`**:
  - A Python script for configuring and building the project using CMake.
  - Compatible with both Windows and Linux.
- **`build_script.bat`**:
  - A batch script for configuring and building the project on Windows.
- **`build_script.sh`**:
  - A shell script for configuring and building the project on Linux.

---

## How to Use

### Verification Script (`verify_models`)
1. Open a terminal in the project directory.
2. Use the appropriate script:
   - **Python**:
     ```bash
     python verify_models.py
     ```
   - **Windows**:
     ```bash
     verify_models.bat
     ```
   - **Linux**:
     ```bash
     ./verify_models.sh
     ```
3. The script will:
   - Locate the Dezyne tool.
   - Verify all `.dzn` files in the `Models` directory.
   - Stop on errors and display the problematic file.

### Build Script (`build_script`)
1. Open a terminal in the project directory.
2. Use the appropriate script:
   - **Python**:
     ```bash
     python build_script.py
     ```
   - **Windows**:
     ```bash
     build_script.bat
     ```
   - **Linux**:
     ```bash
     ./build_script.sh
     ```
3. The script will:
   - Detect the compiler environment.
   - Use CMake to generate build files.
   - Build the Dezyne code generation target.
   - Reconfigure CMake with generated files.
   - Build the final project executable.
4. The compiled executable will be available in the `build` directory.

---

## Troubleshooting

### Common Errors
1. **Compiler Not Found**:
    ```bash
    Error: No suitable compiler detected! Please install MinGW or MSVC.
    ```
    **Solution**:
    - Ensure the compiler is installed and available in your `PATH`.

2. **Dezyne Tool Not Found**:
    ```bash
    Error: Dezyne tool not found at <path_to_dzn_tool>.
    ```
    **Solution**:
    - Verify that `dznPath` in `config.json` points to the Dezyne tool root directory.

3. **Missing Files During Build**:
    ```bash
    Error: Missing files required by generated headers.
    ```
    **Solution**:
    - Verify that all `.dzn` files are correct and complete.
    - Ensure that the Dezyne tool generates all required headers.

---

## Contact
For additional help or to report issues, please open an issue on the project's GitHub repository or contact the project maintainer.
