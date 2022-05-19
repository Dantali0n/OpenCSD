#!/bin/bash

# The passthrough concurrent benchmark measures affected sequential read
# performance due to N incoming parallel writes.
#

for size in 64k 256k 1024k 4096k 16384k 65536k 262144k 1048576k
do
    for bs in 64 256 1024 4096 16384 65536 524288
    do
        echo "fio --size=${size} --bs=${bs} ..."
        for _ in {1..30}
        do
            for _ in {1..4}
                head -c ${size} </dev/urandom > test/test &
            do
            results=$(fio --name=global --rw=randread --size=${size} --bs=${bs} --name=test --output-format=json+)
            echo "bandwidth" > fio-rand-read-${size}-${bs}.csv
            echo "$results" | jq '.jobs[].read.bw_bytes' >> fio-rand-read-${size}-${bs}.csv
        done
    done
done