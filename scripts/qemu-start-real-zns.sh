#!/bin/bash

#ls -l /sys/block/nvme5n1/device/device
#echo "0000:61:00.0" | sudo tee /sys/bus/pci/drivers/nvme/unbind
#sudo modprobe vfio-pci
#lspci -n -s 0000:61:00.0
#echo 1b96 2600 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id

nohup qemu-system-x86_64 -nographic -name qemucsd -m 32G --enable-kvm -cpu host -smp 8 \
-hda ./arch-qemucsd.qcow2 \
-net user,hostfwd=tcp:127.0.0.1:7777-:22,hostfwd=tcp:127.0.0.1:2222-:2000 -net nic \
-device vfio-pci,host=0000:61:00.0 & echo $! > qemu.pid