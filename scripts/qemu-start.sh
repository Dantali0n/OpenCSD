#!/bin/bash

nohup qemu-system-x86_64 -nographic -name qemucsd -m 4G -cpu Haswell -smp 2 \
-hda ./arch-qemucsd.qcow2 \
-net user,hostfwd=tcp::7777-:22,hostfwd=tcp::2222-:2000 -net nic \
-drive file=./znsssd.img,id=mynvme,format=raw,if=none \
-device nvme,serial=baz,id=nvme2,zoned.append_size_limit=131072 \
-device nvme-ns,id=ns2,drive=mynvme,nsid=2,logical_block_size=4096,\
physical_block_size=4096,zoned=true,zoned.zone_size=131072,zoned.zone_capacity=131072,\
zoned.max_open=0,zoned.max_active=0,bus=nvme2 & echo $! > qemu.pid