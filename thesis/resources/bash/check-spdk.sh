mkdir -p test
ld-sudo ./fuse-entry-spdk -- -d -o max_read=2147483647 test &