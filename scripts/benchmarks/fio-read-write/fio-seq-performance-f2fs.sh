#!/bin/bash

fs='f2fs'

for type in write read; do
    for depth in 1 2 4 8 16 32; do
        echo "fio --size=64k --bs=64k --iodepth=${depth} ${type} ..."
        echo "bandwidth" > fio-seq-${type}-${fs}-${depth}-64k.csv
        for _ in {1..30}; do
            results=$(cd /mnt; fio --name=global --rw=${type} --iodepth=${depth} --size=64k --bs=64k --name=fiotest64k --output-format=json+)
            echo "$results" | jq ".jobs[].${type}.bw_bytes" >> fio-seq-${type}-${fs}-${depth}-64k.csv
        done

        echo "fio --size=256k --bs=256k ${type} ..."
        echo "bandwidth" > fio-seq-${type}-${fs}-${depth}-256k.csv
        for _ in {1..30}; do
            results=$(cd /mnt; fio --name=global --rw=${type} --iodepth=${depth} --size=256k --bs=256k --name=fiotest256k --output-format=json+)
            echo "$results" | jq ".jobs[].${type}.bw_bytes" >> fio-seq-${type}-${fs}-${depth}-256k.csv
        done
        for size in 1024k 4096k 16384k 65536k 262144k 1048576k; do
            echo "fio --size=${size} --bs=524288 ${type} ..."
            echo "bandwidth" > fio-seq-${type}-${fs}-${depth}-${size}.csv
            for _ in {1..30}; do
                results=$(cd /mnt; fio --name=global --rw=${type} --iodepth=${depth} --size=${size} --bs=524288 --name=fiotest${size} --output-format=json+)
                echo "$results" | jq ".jobs[].${type}.bw_bytes" >> fio-seq-${type}-${fs}-${depth}-${size}.csv
            done
        done
    done
done