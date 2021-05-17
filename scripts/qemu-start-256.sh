#!/bin/bash

nohup qemu-system-x86_64 -nographic -name qemucsd -m 8G --enable-kvm -cpu host -smp 4 \
-hda ./arch-qemucsd.qcow2 \
-net user,hostfwd=tcp:127.0.0.1:7777-:22,hostfwd=tcp:127.0.0.1:2222-:2000 -net nic \
-drive file=./znsssd.img,id=mynvme,format=raw,if=none \
-device nvme,serial=baz,id=nvme2,zoned.append_size_limit=268435456 \
-device nvme-ns,id=ns2,drive=mynvme,nsid=2,logical_block_size=4096,\
physical_block_size=4096,zoned=true,zoned.zone_size=268435456,zoned.zone_capacity=268435456,\
zoned.max_open=0,zoned.max_active=0,bus=nvme2 & echo $! > qemu.pid