#!/bin/bash

fs='f2fs'

for type in write read; do
    for depth in 1 2 4 8 16 32; d
        for bs in 64 256 1024 4096 16384 65536 524288; do
            echo "fio --size=1048576k --bs=${bs} ${type} ..."
            echo "bandwidth,iops" > fio-rand-${type}-${fs}-${depth}-${bs}.csv
            for _ in {1..30}
            do
                mount /dev/nvme0n1 /mnt || exit 1
                results=$(cd /mnt; fio --name=global --rw=rand${type} --size=1048576k --iodepth=${depth} --bs=${bs} --name=fiotest${bs} --output-format=json+)
                bandwidth=$(echo "$results" | jq ".jobs[].${type}.bw_bytes")
                iops=$(echo "$results" | jq ".jobs[].${type}.iops")
                echo ${bandwidth},${iops} >> fio-rand-${type}-${fs}-${depth}-${bs}.csv
                umount /mnt || exit 1
                mkfs.f2fs -f -m -c /dev/nvme0n2 /dev/nvme0n1 || exit 1
            done
        done
    done
done