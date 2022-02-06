# Scripts

Collection of different scripts for different purposes.

**index**

- [activate](activate) - Environment script to setup paths to libraries, executables and
  aliases.
- [dockerfile](Dockerfile) - File to generate Docker image used
  throughout Gitlab CI pipeline stages.
- [find_func.sh](find_func.sh) - Bash script to find function signatures across
  both static and shared libraries using `nm`.
- [nvme-cli-test.sh](nvme-cli-test.sh) - Used in ZCSD report to compare
  performance from nvme cli.
- [nvme-cli-fill-find-filter.sh](nvme-cli-fill-find-filter.sh) - Used in the
  ZCSD report to compare performance from nvme cli.
- Start the qemu virtual machine using the  downloaded qcow image with emulated
  ZNS SDD. The [activate](activate) environment file needs to be activated
  (using `source`) before this script can be used. Staring qemu also requires
  either a small or large ZNS image to be generated with
 `qemu-img create -f raw znsssd.img 16777216`.
  - [qemu-start.sh](qemu-start.sh) - Use small ZNS image and emulate Haswell
    CPU.
  - [qemu-start-kvm.sh](qemu-start-kvm.sh) - Use small ZNS image with KVM for
    CPU.
  - [qemu-start-256.sh](qemu-start-256.sh) - Use large ZNS Image and emulate
    Haswell CPU.
  - [qemu-start-256-kvm.sh](qemu-start-256-kvm.sh) - Use large ZNS  image with
    KVM for CPU.
- [qemu-stop.sh](qemu-stop.sh) - Stop the QEMU VM