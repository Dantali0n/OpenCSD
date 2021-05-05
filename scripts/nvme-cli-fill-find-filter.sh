#!/bin/bash

time (sudo nvme zns reset-zone -a /dev/nvme0n1; sudo nvme write -s 0 -z 131072 -d integers.dat /dev/nvme0n1; sudo nvme read -s 0 -z 131072 -d data.dat /dev/nvme0n1)
../playground/play-filter