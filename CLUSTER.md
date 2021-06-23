# IEEE Cluster

Zones, LBAs, Integers:

16MB disc image
 * 131072 bytes per zone.
 * 32 zones per LBA.
 * 4096 bytes per LBA.
 * 1024 integers (4 bytes) per LBA.
 * 32768 integers per zone.
 * ~ 16384 integers above RAND_MAX / 2.

1GB disc image
 * 268435456 bytes per zone.
 * 65536 zones per LBA.
 * 4096 bytes per LBA.
 * 1024 integers per LBA
 * 67108864 integers per zone.
 * ~ 33554432 integers above RAND_MAX / 2.

## Experimental setup #1

Measure wall time approach:

- `nvme cli reset-zone` + `nvme cli read > data.dat` + `filter.elf data.dat`
  - Writing of data to file happens on tmpfs (memory backed fs).
  - Reading of file into memory of filter.elf does not account towards total
    execution time.
- spdk reset zones + read first zone + filter C++ native.
  - Application split into 3 separate binaries to make comparison against
    `nvme cli` fair.
- qemu-csd
  - Three separate binaries for fair comparison

## Experimental setup #2

Measure wall time approach consolidated:

- `nvme cli reset-zone -a` + `nvme cli read > data.dat` + `filter.elf data.dat`
    - The zns device is filled with predefined data to simplify.
    - Writing of data to file happens on tmpfs (memory backed fs).
    - Reading of file into memory of filter.elf does not account towards total
      execution time.
- SPDK native single binary
    - Reset all zones, write first zone from predefined data, read first zone,
      filter integers.
    - Use same buffer sizes as qemu-csd binary.
- qemu-csd
    - Resets all zones, write first zone from predefined data, submit BPF program
      to read and filter integers.

In short, we evaluate different approaches to reset, write, read and filter from a
zns SSD. We try to perform the same operations across all solutions taking into
account the sizes of buffers, read and write sizes


```shell
sudo mount -t tmpfs -o size=1024m tmpfs tmp/
cd tmp/
../playground/play-generate-integer-data
nvme zns reset-zone -a /dev/nvme0n1
#nvme write -s 0 -z 131072 -d integers.dat /dev/nvme0n1
nvme zns zone-append -s 0 -z 131072 -d integers.dat /dev/nvme0n1
nvme read -s 0 -c 31 -z 131072 -d data.dat /dev/nvme0n1
../playground/play-filter
```

Create a 1GB image as opposed to 16MB for the 256MB zone tests.

```shell
qemu-img create -f raw znsssd.img 1073741824
```

Inside QEMU mount a tmpfs and generate the integer data file. These
commands will not work due to a limitation in `nvme zns zone-append`.

```shell
sudo mount -t tmpfs -o size=1024m tmpfs tmp/
cd tmp/
../playground/play-generate-integer-data
sudo nvme zns reset-zone -a /dev/nvme0n1
sudo nvme zns zone-append -s 0 -z 268435456 -d integers.dat /dev/nvme0n1
sudo nvme read -s 0 -c 65535 -z 268435456 -d data.dat /dev/nvme0n1
../playground/play-filter
```
  