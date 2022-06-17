#!/bin/bash

# Setup script to perform initialization for benchmarks

cd qemu-csd/build/
source activate
cmake ..
make clean
make fuse-entry-spdk
sudo ../../dependencies/spdk/scripts/setup.sh
mkdir -p test