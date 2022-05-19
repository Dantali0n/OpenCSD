#!/bin/bash

cd qemu-csd/build/
source activate
cmake ..
make fuse-entry-spdk
sudo ../../dependencies/spdk/scripts/setup.sh
mkdir -p test
ld-sudo ./fuse-entry-spdk -- -d -o max_read=2147483647 test &
