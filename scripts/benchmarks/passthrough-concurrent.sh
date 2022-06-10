#!/bin/bash

# The passthrough concurrent benchmark measures affected sequential read
# performance due to N incoming parallel writes.
#

for size in 64k 256k 1024k 4096k 16384k 65536k 262144k 1048576k
do
      echo "passthrough concurrent --size=${size} ..."
      echo "bandwidth" > passthrough-read-concurrent-${size}.csv
      for _ in {1..1}
      do
          ./fuse-entry -- -d -o max_read=2147483647 test > /dev/null 2>&1 &
          pid=$!
          sleep 3
          cp bin/bpf_* test/
          head -c ${size} </dev/urandom > test/test
          for _ in {1..4}
          do
              head -c ${size} </dev/urandom > test/test &
          done
          results=$(python ../../python/csd-read-passthrough.py)
          export size_normal=${size:0:-1}
          echo "$results" | grep "Wall time:" | awk '{print $3}' | awk -F"E" 'BEGIN{OFMT="%10.10f"} {print (ENVIRON["size_normal"] * 1024) / ($1 * (10 ^ $2))}' >> passthrough-read-concurrent-${size}.csv
          kill $pid
      done
done