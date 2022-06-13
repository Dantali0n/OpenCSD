#!/bin/bash

# The passthrough concurrent benchmark measures affected sequential read
# performance due to N incoming parallel writes.
#

cat > fio.job<< EOF
[global]
filename=test/test

[randwrite]
bs=524288
EOF

for type in read write; do
    for concurrent in 1 2 3 4; do
        for size in 1024k 4096k 16384k 65536k 262144k 1048576k; do
            echo "fio sequential concurrent flufflefs ${type} 1-${concurrent} --size=${size} ..."
            echo "bandwidth" > fio-seq-concurrent-${type}-${concurrent}-flufflefs-${size}.csv
            for _ in {1..30}; do
                ./fuse-entry-spdk -- -d -o max_read=2147483647 test > /dev/null 2>&1 &
                fuse_pid=$!
                sleep 5
                head -c ${size} </dev/urandom > test/test || exit 1
                fio --name=global --rw=randwrite --size=${size} --bs=524288 --output-format=json+ --numjobs=${concurrent} fio.job > /dev/null 2>&1 &
                pid=$!
                results=$(fio --name=global --rw=${type} --size=${size} --bs=524288 --output-format=json+ fio.job)
                echo "$results" | jq ".jobs[].${type}.bw_bytes" >> fio-seq-concurrent-${type}-${concurrent}-flufflefs-${size}.csv
                kill $pid
                tail --pid=${pid} -f /dev/null
                kill $fuse_pid || exit 1
                tail --pid=$fuse_pid -f /dev/null
                sleep 5
            done
        done
    done
done
