#!/bin/bash

# Exit on any error
set -e

# Change to the directory of the script
cd "$(dirname "$0")"

# Read dznPath from config.json using jq
if ! DZN_PATH=$(jq -r '.dznPath' config.json); then
    echo "Error: Unable to read Dezyne tool path from config.json."
    exit 1
fi

# Verify that dzn exists
if [ ! -f "$DZN_PATH/dzn" ]; then
    echo "Error: dzn not found in $DZN_PATH."
    exit 1
fi

# Print start message
echo
echo "=== Verifying Models ==="

# Get the number of available processors
NUM_PROCS=$(nproc)
echo "Using $NUM_PROCS threads for verification."

# Process all .dzn files in the Models directory concurrently
find Models -name "*.dzn" | xargs -n 1 -P "$NUM_PROCS" -I {} bash -c '
    echo "Verifying: $(basename "{}")"
    if ! "'$DZN_PATH'"/dzn verify -I Models "{}"; then
        echo "Error verifying $(basename "{}")"
        exit 1
    fi
'

# Print completion message
echo
echo "=== Verification Completed ==="
