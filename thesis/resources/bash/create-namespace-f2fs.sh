sudo nvme create-ns /dev/nvme0 -s 1048576 -c 1048576 -b 4096 --csi=0
sudo nvme create-ns /dev/nvme0 -s 201326592 -c 201326592 -b 4096 --csi=2
sudo nvme attach-ns /dev/nvme0 -n 1 -c 0
sudo nvme attach-ns /dev/nvme0 -n 2 -c 0