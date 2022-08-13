qemu-system-x86_64 -nographic -name qemucsd -m 32G --enable-kvm -cpu host -smp 8 \
-hda ./arch-qemucsd.qcow2 \
-net user,hostfwd=tcp:127.0.0.1:7777-:22,hostfwd=tcp:127.0.0.1:2222-:2000 -net nic \
-device vfio-pci,host=0000:61:00.0