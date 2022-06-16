#!/bin/bash

# The passthrough concurrent benchmark measures affected sequential read
# performance due to N incoming parallel writes.
#

cat > fio.job<< EOF
[global]
filename=test/test

[randwrite]
rw=randwrite
bs=524288
EOF

for concurrent in 1 2 3 4; do
    for size in 1024k 4096k 16384k 65536k 262144k 1048576k; do
        echo "passthrough concurrent 1-${concurrent} --size=${size} ..."
        echo "bandwidth" > passthrough-read-concurrent-${concurrent}-${size}.csv
        for _ in {1..30}; do
            ./fuse-entry-spdk -- -d -o max_read=2147483647 test > /dev/null 2>&1 &
            fuse_pid=$!
            sleep 5
            cp bin/bpf_* test/ || exit 1
            head -c ${size} </dev/urandom > test/test || exit 1
            fio --name=global --rw=randwrite --size=${size} --bs=524288 --output-format=json+ --numjobs=${concurrent} fio.job > /dev/null 2>&1 &
            pid=$!
            results=$(python ../../python/csd-read-passthrough.py)
            export size_normal=${size:0:-1}
            echo "$results" | grep "Wall time:" | awk '{print $3}' | awk -F"E" 'BEGIN{OFMT="%10.10f"} {print (ENVIRON["size_normal"] * 1024) / ($1 * (10 ^ $2))}' >> passthrough-read-concurrent-${concurrent}-${size}.csv
            kill $pid
            tail --pid=${pid} -f /dev/null
            kill $fuse_pid || exit 1
            tail --pid=$fuse_pid -f /dev/null
            sleep 5
        done
    done
done
