git clone https://github.com/Dantali0n/OpenCSD.git
cd opencsd
git checkout tags/thesis
git submodule update --init
mkdir build
cd build
cmake ..
make qemu-build
cmake ..
cd qemu-csd
source activate
qemu-img create -f raw znsssd.img 34359738368
ld-sudo ./qemu-start-256-kvm.sh
git bundle create deploy.git HEAD
# Wait for VM to boot, password: arch
rsync -avz -e "ssh -p 7777" deploy.git arch@localhost:~/
# Type password (arch)
ssh arch@localhost -p 7777
git clone deploy.git qemu-csd
rm deploy.git
cd qemu-csd
git -c submodule."dependencies/qemu".update=none submodule update --init
mkdir build
cd build
cmake -DENABLE_DOCUMENTATION=off -DIS_DEPLOYED=on ..
cmake --build .
cmake ..
sudo shutdown now