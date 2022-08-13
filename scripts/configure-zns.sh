#!/bin/bash

echo mq-deadline | sudo tee /sys/block/nvme0n2/queue/scheduler
sudo mkfs.f2fs -f -m -c /dev/nvme0n2 /dev/nvme0n1