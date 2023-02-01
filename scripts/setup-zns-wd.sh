#!/bin/bash

# Configure ZNS device namespaces

# Total capacity
sudo nvme id-ctrl /dev/nvme1 | grep tnvmcap

# Delete namespaces
sudo nvme delete-ns /dev/nvme1 -n 1
sudo nvme delete-ns /dev/nvme1 -n 2

# Create 4GB conventional namespace with either 512 or 4096 sectors
# sudo nvme create-ns /dev/nvme1 -s 8388608 -c 8388608 -b 512 --csi=0
sudo nvme create-ns /dev/nvme1 -s 1048576 -c 1048576 -b 4096 --csi=0

# Create namespace that requires 4GB of conventional space for F2FS
sudo nvme create-ns /dev/nvme1 -s 212693155 -c 212693155 -b 4096 --csi=2

# Determine remaining capacity
sudo nvme id-ctrl /dev/nvme1 | grep unvmcap

# Attach namespaces
sudo nvme attach-ns /dev/nvme1 -n 1 -c 0
sudo nvme attach-ns /dev/nvme1 -n 2 -c 0

# Create F2FS filesystem
sudo mkfs.f2fs -f -m -c /dev/nvme1n2 /dev/nvme1n1

# Ensure queue scheduler is mq-deadline
echo "mq-deadline" | sudo tee /sys/block/nvme1n2/queue/scheduler
