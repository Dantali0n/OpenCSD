#!/bin/bash

fs='fluffle'

for type in write read; do
    for depth in 1 2 4 8 16 32; do
        for bs in 524288 65536 16384 4096 1024 256 64; do
            echo "fio --size=1048576k --bs=${bs} ${type} ..."
            echo "bandwidth,iops" > fio-rand-${type}-${fs}-${depth}-${bs}.csv
            for _ in {1..30}; do
                ./fuse-entry-spdk -- -d -o max_read=2147483647 test > /dev/null 2>&1 &
                pid=$!
                sleep 3
                results=$(cd test; fio --name=global --rw=rand${type} --iodepth=${depth} --size=1048576k --bs=${bs} --name=fiotest --output-format=json+)
                bandwidth=$(echo "$results" | jq ".jobs[].${type}.bw_bytes")
                iops=$(echo "$results" | jq ".jobs[].${type}.iops")
                echo ${bandwidth},${iops} >> fio-rand-${type}-${fs}-${depth}-${bs}.csv
                kill $pid
                tail --pid=$pid -f /dev/null
            done
        done
    done
done