#!/bin/bash

fs='fluffle'

for type in write read; do
    for depth in 1 2 4 8 16 32; do
        echo "fio --size=64k --bs=64k --iodepth=${depth} ${type} ..."
        echo "bandwidth" > fio-seq-${type}-${fs}-${depth}-64k.csv
        for _ in {1..30}; do
            ./fuse-entry-spdk -- -d -o max_read=2147483647 test > /dev/null 2>&1 &
            pid=$!
            sleep 3
            results=$(cd test; fio --name=global --rw=${type} --iodepth=${depth} --size=64k --bs=64k --name=fiotest --output-format=json+)
            echo "$results" | jq ".jobs[].${type}.bw_bytes" >> fio-seq-${type}-${fs}-${depth}-64k.csv
            kill $pid
        done

        echo "fio --size=256k --bs=256k --iodepth=${depth} ${type} ..."
        echo "bandwidth" > fio-seq-${type}-${fs}-${depth}-256k.csv
        for _ in {1..30}; do
            ./fuse-entry-spdk -- -d -o max_read=2147483647 test > /dev/null 2>&1 &
            pid=$!
            sleep 3
            results=$(cd test; fio --name=global --rw=${type} --iodepth=${depth} --size=256k --bs=256k --name=fiotest --output-format=json+)
            echo "$results" | jq ".jobs[].${type}.bw_bytes" >> fio-seq-${type}-${fs}-${depth}-256k.csv
            kill $pid
        done

        for size in 1024k 4096k 16384k 65536k 262144k 1048576k; do
            echo "fio --size=${size} --bs=524288 --iodepth=${depth} ${type} ..."
            echo "bandwidth" > fio-seq-${type}-${fs}-${depth}-${size}.csv
            for _ in {1..30}; do
                ./fuse-entry-spdk -- -d -o max_read=2147483647 test > /dev/null 2>&1 &
                pid=$!
                sleep 3
                results=$(cd test; fio --name=global --rw=${type} --iodepth=${depth} --size=${size} --bs=524288 --name=fiotest --output-format=json+)
                echo "$results" | jq ".jobs[].${type}.bw_bytes" >> fio-seq-${type}-${fs}-${depth}-${size}.csv
                kill $pid
            done
        done
    done
done