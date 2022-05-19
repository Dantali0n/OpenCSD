#!/bin/bash

fs='fluffle'

for type in write read
    do
    for bs in 64 256 1024 4096 16384 65536 524288
    do
        echo "fio --size=1048576k --bs=${bs} ${type} ..."
        echo "bandwidth" > fio-rand-${type}-${bs}.csv
        for _ in {1..30}
        do
            ./fuse-entry -- -d -o max_read=2147483647 test > /dev/null 2>&1 &
            pid=$!
            sleep 3
            results=$(cd test; fio --name=global --rw=rand${type} --size=1048576k --bs=${bs} --name=fiotest --output-format=json+)
            echo "$results" | jq ".jobs[].${type}.bw_bytes" >> fio-rand-${type}-${bs}.csv
            kill $pid
        done
    done
done