[![pipeline status](https://gitlab.dantalion.nl/vu/qemu-csd/badges/master/pipeline.svg)](https://gitlab.dantalion.nl/vu/qemu-csd/-/pipelines)
[![coverage report](https://gitlab.dantalion.nl/vu/qemu-csd/badges/master/coverage.svg)](https://gitlab.dantalion.nl/vu/qemu-csd/-/jobs/artifacts/master/download?job=coverage)
[![latest commit](https://shields.io/github/last-commit/Dantali0n/qemu-csd)](https://gitlab.dantalion.nl/vu/qemu-csd/-/commits/master)
[![source code license MIT](https://shields.io/github/license/Dantali0n/qemu-csd)](https://gitlab.dantalion.nl/vu/qemu-csd/-/blob/master/LICENSE)
[![follow me on twitter](https://img.shields.io/twitter/follow/D4ntali0n?style=social)](https://twitter.com/D4ntali0n)

## Publications

* FOSDEM, 5 February 2023- [OpenCSD, simple and intuitive computational storage emulation with QEMU and eBPF](https://fosdem.org/2023/schedule/event/csd/)
* thesis, 26 August 2022 - [OpenCSD: LFS enabled Computational Storage Device over Zoned Namespaces (ZNS) SSDs](https://nextcloud.dantalion.nl/index.php/s/CH8sr8YbmwgMxHK/download)
* [ICT.OPEN](https://www.ictopen.nl/home/), 7 April 2022 - [OpenCSD: Unified Architecture for eBPF-powered Computational Storage Devices (CSD) with Filesystem Support](https://gitlab.dantalion.nl/vu/qemu-csd/-/jobs/4591/artifacts/raw/build/ictopen2022.pdf?inline=false)
* arXiv, 13 December 2021 - [Past, Present and Future of Computational Storage: A Survey](https://arxiv.org/abs/2112.09691)
* arXiv, 29 November 2021 - [ZCSD: a Computational Storage Device over Zoned Namespaces (ZNS) SSDs](https://arxiv.org/abs/2112.00142)

# OpenCSD

OpenCSD is an improved version of ZCSD achieving snapshot consistency
log-structured filesystem (LFS) (FluffleFS) integration on Zoned Namespaces
(ZNS) Computational Storage Devices (CSD). Below is a diagram of the overall
architecture as presented to the end user. However, the actual implementation
differs due to the use of emulation using technologies such as QEMU, uBPF and
SPDK.

![](thesis/resources/images/loader-pfs-arch-v3.png)

# FluffleFS

FluffleFS is the filesystem built on using the OpenCSD framework. Designed
based on a LFS with the flash optimized F2FS filesystem as inspiration.
FluffleFS is unique in that it is written in user space code thanks to the
FUSE library while still offering simulated CSD offloading support with
concurrent regular user access to the same file!

## Getting Started

[![asciicast](https://asciinema.org/a/zoM9ncLUTO4QIqntblHKNOEtC.svg)](https://asciinema.org/a/zoM9ncLUTO4QIqntblHKNOEtC)

### Index

* [Directory structure](#directory-structure)
* [Modules](#modules)
* [Dependencies](#dependencies)
* [Setup](#setup)
 * [CMake Configuration](#cmake-configuration)
* [Examples](#examples)
* [Licensing](#licensing)
* [References](#references)

### Directory Structure

* qemu-csd - Project source files
* cmake - Small cmake snippets to enable various features
* dependencies - Project dependencies
* docs - Doxygen generated source code documentation
* [playground]([playground/README.md]) - Small toy examples or other
  experiments
* [python](python/README.md) - Python scripts to aid in visualization or
  measurements
* [scripts](scripts/README.md) - Shell scripts primarily used by CMake to
  install project dependencies
* tests - Unit tests and possibly integration tests
* thesis - Thesis written on OpenCSD using LaTeX
* [zcsd](zcsd/README.md) - Documentation on the previous prototype.
  * compsys 2021 - CompSys 2021 presentation written in LaTeX
  * documentation - Individual Systems Project report written in LaTeX
  * presentation - Individual Systems Project midterm presentation written in
  LaTeX
* .vscode - Launch targets and settings to debug programs running inside QEMU
  over SSH

### Modules

| Module          | Task                                                             |
|-----------------|------------------------------------------------------------------|
| arguments       | Parse commandline arguments to relevant components               |
| bpf_helpers     | Headers to define functions available from within BPF            |
| bpf_programs    | BPF programs ready to run on a CSD using bpf_helpers             |
| fuse_lfs        | Log Structured Filesystem in FUSE                                |
| nvme_csd        | Emulated additional NVMe commands to enable BPF CSDs             |
| nvme_zns        | Interface to handle zoned I/O using abstracted backends          |
| nvme_zns_memory | Non-persistent memory backed emulated ZNS SSD backend            |
| nvme_zns_spdk   | Persistent SPDK backed ZNS SSD backend                           |
| output          | Neatly control messages to stdout and stderr with levels         |
| spdk_init       | Provides SPDK initialization and handles for nvme_zns & nvme_csd |

### Dependencies

This project has a large selection of dependencies as shown below. Note however,
**these dependencies are already available in the image QEMU base image**.

**Warning** Meson must be below version 0.60 due to
[a bug in DPDK](https://bugs.dpdk.org/show_bug.cgi?id=836)

* General
    * Linux 6.0 or higher
    * compiler with c++17 support
    * clang 10 or higher
    * cmake 3.18 or higher
    * python 3.x
    * mesonbuild < 0.60 (`pip3 install meson==0.59`)
    * pyelftools (`pip3 install pyelftools`)
    * ninja
    * cunit
* Documentation
    * doxygen
    * LaTeX
* Code Coverage
    * ctest
    * lcov
    * gcov
    * gcovr
* Continuous Integration
    * valgrind
* Python scripts
    * virtualenv

The following dependencies are automatically compiled and installed into the
build directory.

| Dependency                                                       | Version                                                                                                         |
|------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------|
| [backward](https://github.com/bombela/backward-cpp)              | 1.6                                                                                                             |
| [boost](https://www.boost.org/)                                  | 1.74.0                                                                                                          |
| [bpftool](https://github.com/Netronome/bpf-tool)                 | 5.14                                                                                                            |
| [bpf_load](https://github.com/Netronome/bpf-tool)                | [5.10](https://elixir.bootlin.com/linux/v5.10.77/source/samples/bpf/bpf_load.h)                                 |
| [dpdk](https://www.dpdk.org/)                                    | spdk-21.11                                                                                                      |
| [generic-ebpf](https://github.com/generic-ebpf/generic-ebpf)     | [c9cee73](https://github.com/generic-ebpf/generic-ebpf/commit/c9cee73c73845c9d60aef807b7ee7891987cd6fd)         |
| [fuse-lfs](https://github.com/sphurti/Log-Structured-Filesystem) | [526454b](https://github.com/sphurti/Log-Structured-Filesystem/commit/526454b99102d4e8875898550f92d577bbbb8ca2) |
| [libbpf](https://github.com/libbpf/libbpf)                       | 0.5                                                                                                             |
| [libfuse](https://github.com/libfuse/libfuse)                    | 3.10.5                                                                                                          |
| [libbpf-bootstrap](https://github.com/libbpf/libbpf)             | [67a29e5](https://github.com/libbpf/libbpf-bootstrap/commit/67a29e511cc9d0a570d4d3b9797827f3a08ccdb5)           |
| [linux](https://www.kernel.org/)                                 | 5.14                                                                                                            |
| [spdk](https://github.com/spdk/spdk)                             | 22.09                                                                                                           |
| [isa-l](https://github.com/intel/isa-l)                          | spdk-v2.30.0                                                                                                    |
| [rocksdb](https://github.com/facebook/rocksdb)                   | 6.25.3                                                                                                          |
| [qemu](https://www.qemu.org/)                                    | 6.1.0                                                                                                           |
| [uBPF](https://github.com/iovisor/ubpf)                          | [9eb26b4](https://github.com/iovisor/ubpf/commit/9eb26b4bfdec6cafbf629a056155363f12cec972)                      |
| [xenium](https://github.com/mpoeter/xenium/)                     | [f1d28d0](https://github.com/mpoeter/xenium/commit/f1d28d0980cf2128c3f6b77d321aad5ca469dbce)                    |

### Setup

The project requires between 15 and 30 GB of disc space depending on
your configuration. While there are no particular system memory or performance
requirements for running OpenCSD, debugging requires between 10 and 16 GB of
reserved system memory. The table shown below explains the differences between
the  possible configurations and their requirements.

| Storage Mode   | Debugging | Disc space | System Memory | Cmake Parameters                                               |
|----------------|-----------|------------|---------------|----------------------------------------------------------------|
| Non-persistent | No        | 15 GB      | < 2 GB        | -DCMAKE_BUILD_TYPE=Release -DIS_DEPLOYED=on -DENABLE_TESTS=off |                                
| Non-persisten  | Yes       | 15 GB      | 13 GB         | -DCMAKE_BUILD_TYPE=Debug -DIS_DEPLOYED=on                      |
| Persistent     | No        | 30 GB      | 10 GB         | -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=off                  |
| Persistent     | Yes       | 30 GB      | 16 GB         | default                                                        |

OpenCSD its initial configuration and compilation must be performed prior to
its use. After checking out the OpenCSD repository this can be achieved by
executing the commands shown below. Each section of individual commands must be
executed from the root of the project directory.

```shell script
git submodule update --init
mkdir build
cd build
cmake .. # For non default configurations copy the cmake parameters before the ..
cmake --build .
# Do not use make -j $(nproc), CMake is not able to solve concurrent dependency chain
cmake .. # this prevents re-compiling dependencies on every next make command
```

```shell script
cd build/qemu-csd
source activate
qemu-img create -f raw znsssd.img 34359738368 # 16777216
# By default qemu will use 4 CPU cores and 8GB of memory
./qemu-start-256-kvm.sh
# Wait for QEMU VM to fully boot... (might take some time)
git bundle create deploy.git HEAD
# Type password (arch)
rsync -avz -e "ssh -p 7777" deploy.git arch@localhost:~/
# Type password (arch)
ssh arch@localhost -p 7777
git clone deploy.git qemu-csd
rm deploy.git
cd qemu-csd
git -c submodule."dependencies/qemu".update=none submodule update --init
mkdir build
cd build
cmake -DENABLE_DOCUMENTATION=off -DIS_DEPLOYED=on ..
# Do not use make -j $(nproc), CMake is not able to solve concurrent dependency chain
cmake --build .
```

### Environment:
Within the build folder will be a `qemu-csd/activate` script. This script can be
sourced using any shell `source qemu-csd/activate`. This script configures
environment variables such as `LD_LIBRARY_PATH` while also exposing an essential
sudo alias: `ld-sudo`.

The environment variables ensure any linked libraries can be found for targets
compiled by Cmake. Additionally, `ld-sudo` provides a mechanism to start targets
with sudo privileges while retaining these environment variables. The
environment can be deactivated at any time by executing `deactivate`.

### Usage Examples:


1. Start the filesystem in a memory backed mode and mount it on `test`.

```
cd build
make fuse-entry
cmake ..
cd qemu-csd
mkdir −p test
source activate
ld−sudo ./fuse−entry −− −d −o max_read=2147483647 test &
```

### Contributing

#### CMake Configuration

This section documents all configuration parameters that the CMake project
exposes and how they influence the project. For more information about the
CMake project see the report generated from the documentation folder. Below 
all parameters are listed along their default value and a brief description.

| Parameter            | Default | Use case                                         |
|----------------------|---------|--------------------------------------------------|
| ENABLE_TESTS         | ON      | Enables unit tests and adds tests target         |
| ENABLE_CODECOV       | OFF     | Produce code coverage report \w unit tests       |
| ENABLE_DOCUMENTATION | ON      | Produce code documentation using doxygen & LaTeX |
| ENABLE_PLAYGROUND    | OFF     | Enables playground targets                       |
| ENABLE_LEAK_TESTS    | OFF     | Add compile parameter for address sanitizer      |
| IS_DEPLOYED          | OFF     | Indicate that CMake project is deployed in QEMU  |

For several parameters a more in depth explanation is required, primarily
_IS_DEPLOYED_. This parameter is used as the CMake project is both used to
compile QEMU and configure it as well as compile binaries to run inside QEMU. As
a results, the CMake project needs to be able to identify if it is being
executed outside of QEMU or not. This is what _IS_DEPLOYED_ facilitates.
Particularly, _IS_DEPLOYED_ prevents the compilation of QEMU from source.

### Licensing

This project is available under the MIT license, several limitations apply
including:
  
* Source files with an alternative author or license statement other than Dantali0n and MIT respectively.
* Images subject to copyright or usage terms, such the VU and UvA logo.
* CERN beamer template files by Jerome Belleman.
* Configuration files that can't be subject to licensing such as `doxygen.cnf`
  or `.vscode/launch.json`

### References

* ZNS
  * [Zoned storage ZNS SSDs introduction](https://zonedstorage.io/introduction/zns/)
  * [Getting started with ZNS in QEMU](https://www.snia.org/educational-library/getting-started-nvme-zns-qemu-2020)
  * [NVMe ZNS command set 1.0 ratified TP](https://nvmexpress.org/wp-content/uploads/NVM-Express-1.4-Ratified-TPs-1.zip)
  * [libnvme presentation](https://www.usenix.org/sites/default/files/conference/protected-files/vault20_slides_busch.pdf)
  * [dm-zap conventional zones for ZNS](https://github.com/westerndigitalcorporation/dm-zap)
  * [FEMU accurate NVMe SSD Emulator](https://github.com/ucare-uchicago/FEMU)
* Filesystems
  * [Linux Inode](https://man7.org/linux/man-pages/man7/inode.7.html)
  * Filesystem Benchmarks
    * [Filebench](https://github.com/filebench/filebench)
    * [Filebench Tutorial](http://www.nfsv4bat.org/Documents/nasconf/2005/mcdougall.pdf)
* FUSE
  * [To FUSE or Not to FUSE: Performance of User-Space File Systems](http://libfuse.github.io/doxygen/fast17-vangoor.pdf)
  * [FUSE kermel documentation](https://www.kernel.org/doc/html/latest/filesystems/fuse.html)
  * [FUSE forget](https://fuse-devel.narkive.com/SMANJULN/when-does-fuse-forget)
  * Other FUSE3 filesystems that can be used for reference
    * [MergerFS](https://github.com/trapexit/mergerfs/tree/master/src)
* LFS
  * [f2fs usenix paper](https://www.usenix.org/system/files/conference/fast15/fast15-paper-lee.pdf)
  * [f2fs kernel documentation](https://www.kernel.org/doc/html/latest/filesystems/f2fs.html)
* BPF
  * Linux Kernel related
    * [Linux bpf manpage](https://www.man7.org/linux/man-pages/man2/bpf.2.html)
    * [BPF kernel documentation](https://www.kernel.org/doc/Documentation/networking/filter.txt)
  * BPF-CO-RE & BTF
    * [Linux BTF documentation](https://www.kernel.org/doc/html/latest/bpf/btf.html)
    * [BPF portability and CO-RE](https://facebookmicrosites.github.io/bpf/blog/2020/02/19/bpf-portability-and-co-re.html)  **Highly Recommended Read**
  * libbpf / standalone related
    * [BCC to libbpf conversion](https://facebookmicrosites.github.io/bpf/blog/2020/02/20/bcc-to-libbpf-howto-guide.html)
    * [Cilium BPF + XDP reference guide](https://docs.cilium.io/en/v1.9/bpf/) **Highly Recommended Read**
    * bpf_load
      * [Linux Observability with BPF](https://www.oreilly.com/library/view/linux-observability-with/9781492050193/)
    * bpf-bootstrap
      * [Building BPF applications with libbpf-bootstrap](https://nakryiko.com/posts/libbpf-bootstrap/)
  * Userspace BPF execution / interpretation
    * [uBPF](https://github.com/iovisor/ubpf)
    * [iomartin uBPF patch expose registers](https://github.com/iomartin/ubpf/commit/ca1ad94613a01e1fa5cc04d43c73acc6b5074881)
    * [iomartin uBPF patch relocation type](https://github.com/iomartin/ubpf/commit/af4a54c201524f975137d8c531dfef82010b65cd)
    * [generic-ebpf](https://github.com/generic-ebpf/generic-ebpf)
  * Verifiers
    * [PREVAIL](https://github.com/vbpf/ebpf-verifier)
  * Hardware implementations
    * [hBPF](https://github.com/rprinz08/hBPF)
  * Various
    * [BTF sysfs vmlinux ABI](https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-kernel-btf)
    * [BPF features and minimal kernel versions](https://github.com/iovisor/bcc/blob/master/docs/kernel-versions.md)
    * [BPF Performance Tools (Chapters 1, 2, 17.1, 17.5, 18)](http://www.brendangregg.com/bpf-performance-tools-book.html)
    * [eBPF release artile lwn.net](https://lwn.net/Articles/603983/)
    * [Why pingCAP switched from BCC to libbpf](https://pingcap.com/blog/why-we-switched-from-bcc-to-libbpf-for-linux-bpf-performance-analysis)
* Repositories / Libraries
  * [uNVME](https://github.com/OpenMPDK/uNVMe)
  * [SPDK](https://github.com/spdk/spdk)
* Patchsets
  * [ZNS SSD QEMU patch v11](http://patchwork.ozlabs.org/project/qemu-devel/list/?series=219344)
  * [ZNS SSD QEMU patch v2](https://patchwork.kernel.org/project/qemu-devel/cover/20200617213415.22417-1-dmitry.fomichev@wdc.com/)
