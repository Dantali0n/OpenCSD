sudo nvme create-ns /dev/nvme0 -s 1934622720 -c 1934622720 -b 4096 --csi=2
sudo nvme attach-ns /dev/nvme0 -n 1 -c 0