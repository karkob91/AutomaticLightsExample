import os
import subprocess
import sys
from pathlib import Path

def detect_compiler():
    """Detect the available compiler and return the CMake generator."""
    print("=== Detecting Compiler Environment ===")
    try:
        subprocess.run(["gcc", "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        print("Detected MinGW environment.")
        return "MinGW Makefiles"
    except subprocess.CalledProcessError:
        pass  # GCC not found

    try:
        subprocess.run(["cl"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        print("Detected MSVC environment.")
        return "Visual Studio 17 2022"
    except subprocess.CalledProcessError:
        pass  # MSVC not found

    print("Error: No suitable compiler detected! Please install MinGW or MSVC.")
    sys.exit(1)

def run_command(command, description):
    """Run a shell command and handle errors."""
    print(f"=== {description} ===")
    try:
        subprocess.run(command, check=True, text=True)
    except subprocess.CalledProcessError as e:
        print(f"Error during {description.lower()}: {e}")
        sys.exit(e.returncode)

def main():
    # Paths
    script_dir = Path(__file__).parent.resolve()
    build_dir = script_dir / "build"
    num_processors = os.cpu_count() or 1

    print(f"WARNING: Using {num_processors} processors for the build.")
    print("Adjust the script if a different number is desired.")

    # Step 1: Detect compiler and set CMake generator
    generator = detect_compiler()

    # Step 2: Run CMake to generate build files
    run_command(
        ["cmake", "-G", generator, "-B", str(build_dir), "-S", str(script_dir)],
        "Running CMake Configuration"
    )

    # Step 3: Build Dezyne code generation target
    run_command(
        ["cmake", "--build", str(build_dir), "--target", "generate_dezyne_code", "--", f"-j{num_processors}"],
        "Building Dezyne Code Generation Target"
    )

    # Step 4: Reconfigure CMake with generated files
    run_command(
        ["cmake", "-B", str(build_dir), "-S", str(script_dir)],
        "Reconfiguring CMake with Generated Files"
    )

    # Step 5: Final project build
    run_command(
        ["cmake", "--build", str(build_dir), "--", f"-j{num_processors}"],
        "Building the Final Project"
    )

    print("=== Build Completed Successfully ===")

if __name__ == "__main__":
    main()
