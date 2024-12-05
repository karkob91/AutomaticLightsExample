import os
import subprocess
import sys
import json
from pathlib import Path

def load_config(config_path):
    """Load and parse the JSON configuration file."""
    try:
        with open(config_path, 'r') as f:
            config = json.load(f)
            return config
    except (FileNotFoundError, json.JSONDecodeError) as e:
        print(f"Error loading configuration file: {e}")
        sys.exit(1)

def detect_compiler():
    """Detect the available compiler and set the appropriate generator."""
    print("=== Detecting Compiler Environment ===")
    if sys.platform.startswith("linux") or sys.platform.startswith("darwin"):  # Linux or MacOS
        if subprocess.run(["gcc", "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0:
            print("Detected GCC environment.")
            return "Unix Makefiles"
        else:
            print("Error: GCC not found! Please install GCC.")
            sys.exit(1)
    elif sys.platform == "win32":  # Windows
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
    else:
        print("Error: Unsupported platform.")
        sys.exit(1)

def run_cmake(generator, build_dir, source_dir):
    """Run CMake to configure and generate build files."""
    try:
        subprocess.run(
            ["cmake", "-G", generator, "-B", build_dir, "-S", source_dir],
            check=True
        )
    except subprocess.CalledProcessError as e:
        print(f"Error during CMake configuration: {e}")
        sys.exit(1)

def build_target(build_dir, target, processors):
    """Run CMake to build a specific target."""
    print(f"=== Building Target: {target} ===")
    try:
        subprocess.run(
            ["cmake", "--build", build_dir, "--target", target, "--", "-j", str(processors)],
            check=True
        )
    except subprocess.CalledProcessError as e:
        print(f"Error building target {target}: {e}")
        sys.exit(1)

def main():
    script_dir = Path(__file__).parent.resolve()
    config_path = script_dir / "config.json"
    build_dir = script_dir / "build"

    # Load configuration
    config = load_config(config_path)
    project_name = config.get("projectName")
    dzn_path = config.get("dznPath")
    runtime_path = config.get("dezyneRuntimePath")

    if not project_name or not dzn_path or not runtime_path:
        print("Error: 'projectName', 'dznPath', and 'dezyneRuntimePath' must be specified in config.json.")
        sys.exit(1)

    print(f"Project Name: {project_name}")
    print(f"Dezyne Path: {dzn_path}")
    print(f"Runtime Path: {runtime_path}")

    # Detect compiler and set CMake generator
    generator = detect_compiler()

    # Run CMake configuration
    print("=== Running CMake Configuration ===")
    run_cmake(generator, build_dir, script_dir)

    # Build the Dezyne code generation target
    processors = os.cpu_count() or 1
    build_target(build_dir, "generate_dezyne_code", processors)

    # Re-run CMake to include generated files
    print("=== Re-running CMake Configuration with Generated Files ===")
    run_cmake(generator, build_dir, script_dir)

    # Build the final project
    build_target(build_dir, "all", processors)

    print("=== Build Completed Successfully ===")

if __name__ == "__main__":
    main()
