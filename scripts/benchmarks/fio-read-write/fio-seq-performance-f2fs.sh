#!/bin/bash

fs='f2fs'

for type in write read
    do
    echo "fio --size=64k --bs=64k ${type} ..."
    echo "bandwidth" > fio-seq-${type}-${fs}-64k.csv
    for _ in {1..30}
    do
        results=$(fio --name=global --rw=${type} --size=64k --bs=64k --name=fiotest --output-format=json+)
        echo "$results" | jq ".jobs[].${type}.bw_bytes" >> fio-seq-${type}-${fs}-64k.csv
    done

    echo "fio --size=256k --bs=256k ${type} ..."
    echo "bandwidth" > fio-seq-${type}-${fs}-256k.csv
    for _ in {1..30}
    do
        results=$(fio --name=global --rw=${type} --size=256k --bs=256k --name=fiotest --output-format=json+)
        echo "$results" | jq ".jobs[].${type}.bw_bytes" >> fio-seq-${type}-${fs}-256k.csv
    done

    for size in 1024k 4096k 16384k 65536k 262144k 1048576k
    do
        echo "fio --size=${size} --bs=524288 ${type} ..."
        echo "bandwidth" > fio-seq-${type}-${fs}-${size}.csv
        for _ in {1..30}
        do
            results=$(fio --name=global --rw=${type} --size=${size} --bs=524288 --name=fiotest --output-format=json+)
            echo "$results" | jq ".jobs[].${type}.bw_bytes" >> fio-seq-${type}-${fs}-${size}.csv
        done
    done
done