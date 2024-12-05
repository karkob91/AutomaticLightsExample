import os
import subprocess
import sys
import json
from concurrent.futures import ThreadPoolExecutor, as_completed


def load_config(config_file):
    """Load the JSON configuration file."""
    try:
        with open(config_file, "r") as file:
            config = json.load(file)
        return config
    except (FileNotFoundError, json.JSONDecodeError) as e:
        print(f"Error loading configuration file {config_file}: {e}")
        sys.exit(1)


def verify_file(dzn_tool, models_dir, file_path):
    """Verify a single .dzn file."""
    try:
        subprocess.run(
            ["cmd.exe", "/c", dzn_tool, "verify", "-I", models_dir, file_path],
            check=True
        )
        print(f"Verified: {file_path}")
    except subprocess.CalledProcessError:
        model_name = os.path.basename(file_path)
        print(f"Error verifying model: {model_name}")
        sys.exit(1)  # Exit the script if verification fails


def main():
    # Get the directory of the current script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    config_file = os.path.join(script_dir, "config.json")  # JSON configuration file
    models_dir = os.path.join(script_dir, "models")
    
    # Load configuration
    config = load_config(config_file)
    dzn_path = config.get("dznPath")
    if not dzn_path:
        print("Error: 'dznPath' not found in configuration file.")
        sys.exit(1)
    
    # Full path to dzn.cmd
    dzn_tool = os.path.join(dzn_path, "dzn.cmd")

    # Check if the dzn tool exists
    if not os.path.isfile(dzn_tool):
        print(f"Error: dzn tool not found at {dzn_tool}.")
        sys.exit(1)

    # Find all .dzn files in the models directory
    dzn_files = []
    for root, _, files in os.walk(models_dir):
        for file in files:
            if file.endswith(".dzn"):
                file_path = os.path.join(root, file)
                dzn_files.append(file_path)

    if not dzn_files:
        print("No .dzn files found in the models directory.")
        return

    # Detect the number of available CPU cores
    num_cores = os.cpu_count() or 1
    print(f"Using {num_cores} threads for verification.")

    # Use ThreadPoolExecutor to verify files in parallel
    with ThreadPoolExecutor(max_workers=num_cores) as executor:
        future_to_file = {
            executor.submit(verify_file, dzn_tool, models_dir, file): file
            for file in dzn_files
        }

        # Wait for all tasks to complete
        for future in as_completed(future_to_file):
            try:
                future.result()  # Raise exception if occurred
            except Exception as e:
                # Exit immediately for any error
                sys.exit(1)


if __name__ == "__main__":
    main()
