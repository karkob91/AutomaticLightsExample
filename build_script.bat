@echo off
setlocal

REM === Detect Compiler Environment ===
echo === Detecting Compiler Environment ===
where gcc >nul 2>&1
if %errorlevel% equ 0 (
    set GENERATOR="MinGW Makefiles"
    set COMPILER="MinGW"
    echo Detected MinGW environment.
) else (
    where cl >nul 2>&1
    if %errorlevel% equ 0 (
        set GENERATOR="Visual Studio 17 2022"
        set COMPILER="MSVC"
        echo Detected MSVC environment.
    ) else (
        echo Error: No suitable compiler detected! Please install MinGW or MSVC.
        exit /b 1
    )
)

REM === Processor Count Warning ===
echo WARNING: Using %NUMBER_OF_PROCESSORS% processors for the build.
echo Adjust the script if a different number is desired.

REM === Step 1: Run CMake to Generate Build Files ===
echo === Running CMake Configuration ===
cmake -G %GENERATOR% -B build -S .
if %errorlevel% neq 0 (
    echo Error: CMake configuration failed.
    exit /b %errorlevel%
)

REM === Step 2: Build Dezyne Code Generation Target ===
echo === Building Dezyne Code Generation Target ===
cmake --build build --target generate_dezyne_code -- -j %NUMBER_OF_PROCESSORS%
if %errorlevel% neq 0 (
    echo Error: Dezyne code generation failed.
    exit /b %errorlevel%
)

REM === Step 3: Reconfigure CMake with Generated Files ===
echo === Reconfiguring CMake with Generated Files ===
cmake -B build -S .
if %errorlevel% neq 0 (
    echo Error: Reconfiguration failed.
    exit /b %errorlevel%
)

REM === Step 4: Final Project Build ===
echo === Building the Final Project ===
cmake --build build -- -j %NUMBER_OF_PROCESSORS%
if %errorlevel% neq 0 (
    echo Error: Final build failed.
    exit /b %errorlevel%
)

REM === Completion Message ===
echo === Build Completed Successfully ===
exit /b 0
