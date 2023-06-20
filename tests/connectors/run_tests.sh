#!/bin/bash

cleanup () {
  echo "Cleaning up generated files..."
  find . -name "test" -type d -exec rm -r {} \;
  echo "Cleanup complete."
}

# Register the cleanup function to be called on exit
trap cleanup EXIT

for dir in */; do
  cd "$dir"

  # Call vitis_hls with the script.tcl file and save the return status
  status=$(vitis_hls -f script.tcl)

  echo "vitis_hls return status for $dir: $status"

  cd ..
done