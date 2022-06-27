#!/bin/bash

# The entropy benchmark measures cpu time uses across OS domains

for size in 1024k 4096k 16384k 65536k 262144k 1048576k
do
    echo "entropy kernel --size=${size} ..."
    echo "real,user,sys" > entropy-kernel-${size}.csv
    for _ in {1..30}
    do
        ./fuse-entry -- -d -o max_read=2147483647 test > /dev/null 2>&1 &
        pid=$!
        sleep 5
        cp bin/bpf_* test/ || exit 1
        head -c ${size} < /dev/urandom > test/test || exit 1
        results=$({ time python ../../python/csd-entropy-passthrough.py; } 2>&1)
        real=$(echo "$results" | grep real | awk '{print $2}')
        user=$(echo "$results" | grep user | awk '{print $2}')
        sys=$(echo "$results" | grep sys | awk '{print $2}')
        echo "${real},${user},${sys}" >> entropy-kernel-${size}.csv
        kill $pid || exit 1
        tail --pid=$pid -f /dev/null
    done
done