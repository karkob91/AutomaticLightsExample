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

# Process all .dzn files in the models directory
find models -name "*.dzn" | while read -r dzn_file; do
    echo "Verifying: $(basename "$dzn_file")"
    if ! "$DZN_PATH/dzn" verify -I models "$dzn_file"; then
        echo "Error verifying $(basename "$dzn_file")"
        exit 1
    fi
done

# Print completion message
echo
echo "=== Verification Completed ==="
