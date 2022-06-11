#!/bin/bash

# The passthrough sequential benchmark measures sequential read
# performance for a passthrough kernel

for size in 1024k 4096k 16384k 65536k 262144k 1048576k
do
    echo "passthrough sequential --size=${size} ..."
    echo "bandwidth" > passthrough-read-${size}.csv
    for _ in {1..30}
    do
        ./fuse-entry-spdk -- -d -o max_read=2147483647 test > /dev/null 2>&1 &
        pid=$!
        sleep 5
        cp bin/bpf_* test/ || exit 1
        head -c ${size} < /dev/urandom > test/test || exit 1
        results=$(python ../../python/csd-read-passthrough.py)
        export size_normal=${size:0:-1}
        echo "$results" | grep "Wall time:" | awk '{print $3}' | awk -F"E" 'BEGIN{OFMT="%10.10f"} {print (ENVIRON["size_normal"] * 1024) / ($1 * (10 ^ $2))}' >> passthrough-read-${size}.csv
        kill $pid || exit 1
        tail --pid=$pid -f /dev/null
    done
done