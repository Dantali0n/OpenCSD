#!/bin/bash

fs='f2fs'

for type in write read
    do
    for bs in 64 256 1024 4096 16384 65536 524288
    do
        echo "fio --size=1048576k --bs=${bs} ${type} ..."
        echo "bandwidth" > fio-rand-${type}-${fs}-${bs}.csv
        for _ in {1..30}
        do
            results=$(cd /mnt; fio --name=global --rw=rand${type} --size=1048576k --bs=${bs} --name=fiotest${bs} --output-format=json+)
            echo "$results" | jq ".jobs[].${type}.bw_bytes" >> fio-rand-${type}-${fs}-${bs}.csv
        done
    done
done