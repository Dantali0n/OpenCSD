# Identify the ZNS device for passthrough
sudo nvme list
# Find the pci address of the target device
ls -l /sys/block/nvme5n1/device/device
# Unbind the nvme driver
echo "0000:61:00.0" | sudo tee /sys/bus/pci/drivers/nvme/unbind
# Enable vfio-pci
sudo modprobe vfio-pci
# Get the vendor and device id
lspci -n -s 0000:61:00.0
# Enable the vendor and device id for vfio-pci
echo 1b96 2600 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id