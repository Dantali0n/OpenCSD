[![pipeline status](https://gitlab.dantalion.nl:4443/vrije-universiteit-vu-/qemu-csd/badges/master/pipeline.svg)](https://gitlab.dantalion.nl:4443/vrije-universiteit-vu-/qemu-csd/commits/master)
[![coverage report](https://gitlab.dantalion.nl:4443/vrije-universiteit-vu-/qemu-csd/badges/master/coverage.svg)](https://gitlab.dantalion.nl:4443/vrije-universiteit-vu-/qemu-csd/commits/master)
# QEMU-CSD



### Project goals

* Week 1
  * Perform read / write requests on ZNS SSD in QEMU.
  * Setup Github repository and bi-directional mirroring.
  * Read blockNDP paper.
* Week 2
  * Setup Cmake target to deploy binary into QEMU vm.
  * SPDK hello world.
  * Unit tests with basic SPDK.

### Index

* [Directory structure](#directory-structure)
* [Modules](#modules)
* [Dependencies](#dependencies)
* [Setup](#setup)
* [Configuration](#configuration)
* [Licensing](#licensing)
* [References](#references)
* [Snippets](#snippets)

### Directory structure

* qemucsd - project source files
* cmake - small cmake snippets to enable various features
* dependencies - project dependencies
* docs - doxygen generated source code documentation
* documentation - project documentation written in LaTeX
* [playground]([playground/README.md]) - small toy examples or other experiments
* [python](python/README.md) - python scripts to aid in visualization or measurements
* tests - unit tests and possibly integration tests

### Modules

| Module     | Optional | Task                                               |
|------------|----------|----------------------------------------------------|
| arguments  | Yes      | Parse commandline arguments to relevant components |

#### Dependencies

This project requires quite some dependencies, the
majority will be compiled by the project itself and installed
into the build directory. Anything that is not automatically
compiled and linked is shown below.

* General
    * compiler with c++17 support
    * cmake 3.13 or higher
* Windows specific
    * Visual Studio 2019 community
    * cygwin
* Documentation
    * doxygen
    * LaTeX
* Code Coverage
    * ctest
    * lcov
    * gcov
* Continuous Integration
    * valgrind
* Python scripts
    * python 3.x
    * virtualenv
* QEMU
    * librdmacm
    * libibverbs
    * libibumad

The following dependencies are automatically compiled. Dependencies are preferably
linked statically due to the nature of this project. However, for several dependencies
this is not possible due to various reason. For Boost, it is because the unit test
framework can not be statically linked (easily):

| Dependency                                                         | Version                                                       |
|--------------------------------------------------------------------|---------------------------------------------------------------|
| [backward](https://github.com/bombela/backward-cpp)                | 1.5                                                           |
| [booost](https://www.boost.org/)                                   | 1.74.0                                                        |
| [libbpf](https://github.com/libbpf/libbpf)                         | 0.3                                                           |
| [spdk](https://github.com/spdk/spdk)                               | 2.10                                                          |
| [qemu](https://www.qemu.org/)                                      | [nvme-next d79d797b0d](git://git.infradead.org/qemu-nvme.git) |

#### Setup

Building tools and dependencies is done by simply executing the following commands
from the root directory. For a more complete list of cmake options see the
[Configuration](#configuration) section.

This first section of commands generates targets for host development. Among
these is compiling and downloading an image for qemu. Many parts of this project
can be developed on the host but some require being developed on the guest. See
the next section for on guest development.

```shell script
git submodule update --init --recursive
mkdir build
cd build
cmake ..
cmake --build .
cmake .. # this prevents re-compiling dependencies on every next make command
source qemu-csd/activate.sh
# run commands and tools as you please for host based development
deactivate
```

From the root directory execute the following commands for the one time
deployment into the qemu guest. These command assume the previous section of
commands has successfully been executed.

```shell
git bundle create deploy.git HEAD
cd build/qemu-csd
source activate.sh
qemu-img create -f raw znsssd.img 16777216
./qemu-start.sh
# Wait for QEMU VM to fully boot... (might take some time)
rsync -avz -e "ssh -p 7777" deploy.git arch@localhost:~/
# Type password (arch)
ssh arch@localhost -p 7777
# Type password (arch)
git clone deploy.git qemu-csd
rm deploy.git
cd qemu-csd
git -c submodule."dependencies/qemu".update=none submodule update --init
mkdir build
cd build
cmake -DENABLE_DOXYGEN=off -DIS_DEPLOYED=on ..
```

Additionally, any python based tools and graphs are generated by execution these
additional commands from the root directory. Ensure the previous environment has
been deactivated.

```shell script
virtualenv -p python3 python
cd python
source bin/activate
pip install -r requirements.txt
```

#### Configuration

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
_IS_DEPLOYED_. This parameter is used as the Cmake project is both used to
compile QEMU and configure it as well as compile binaries to run inside QEMU. As
a results, the CMake project needs to be able to identify if it is being
executed outside of QEMU or not. This is what _IS_DEPLOYED_ facilitates. The
user typically **does not** have to interact with this. This is because the
CMake project will automatically be deployed into QEMU and reconfigured, if
CMake is being run with _IS_DEPLOYED_ off. However, it is of course still
important to understand when to use this parameter if changes are made to the
CMake source files.

#### Licensing

Some files are licensed under a variety of different licenses please see
specific source files for licensing details.

#### References

* [SPDK](https://spdk.io/)
* [Zoned storage ZNS SSDs introduction](https://zonedstorage.io/introduction/zns/)
* [Getting started with ZNS in QEMU](https://www.snia.org/educational-library/getting-started-nvme-zns-qemu-2020)
* [NVMe ZNS command set 1.0 ratified TP](https://nvmexpress.org/wp-content/uploads/NVM-Express-1.4-Ratified-TPs-1.zip)
* Repositories / Libraries
  * [uNVME](https://github.com/OpenMPDK/uNVMe)
  * [SPDK](https://github.com/spdk/spdk)
* Patchsets
  * [ZNS SSD QEMU patch v11](http://patchwork.ozlabs.org/project/qemu-devel/list/?series=219344)
  * [ZNS SSD QEMU patch v2](https://patchwork.kernel.org/project/qemu-devel/cover/20200617213415.22417-1-dmitry.fomichev@wdc.com/)

#### Snippets

* SPDK -> now supports ZNS zone append
* uNVME
* OCSSD
* RMDA
* libbpf
* libbpf-tools
* Linux Kernel:
  * p2pdma 
  * ioctl

Configuration and parameters for QEMU ZNS SSDs:
```shell
Usage:
      -device nvme-subsys,id=subsys0
      -device nvme,serial=foo,id=nvme0,subsys=subsys0
      -device nvme,serial=bar,id=nvme1,subsys=subsys0
      -device nvme,serial=baz,id=nvme2,subsys=subsys0
      -device nvme-ns,id=ns1,drive=<drv>,nsid=1,subsys=subsys0  # Shared
      -device nvme-ns,id=ns2,drive=<drv>,nsid=2,bus=nvme2

nvme options:
  addr=<int32>           - Slot and optional function number, example: 06.0 or 06 (default: -1)
  aer_max_queued=<uint32> -  (default: 64)
  aerl=<uint8>           -  (default: 3)
  cmb_size_mb=<uint32>   -  (default: 0)
  discard_granularity=<size> -  (default: 4294967295)
  drive=<str>            - Node name or ID of a block device to use as a backend
  failover_pair_id=<str>
  logical_block_size=<size> - A power of two between 512 B and 2 MiB (default: 0)
  max_ioqpairs=<uint32>  -  (default: 64)
  mdts=<uint8>           -  (default: 7)
  min_io_size=<size>     -  (default: 0)
  msix_qsize=<uint16>    -  (default: 65)
  multifunction=<bool>   - on/off (default: false)
  num_queues=<uint32>    -  (default: 0)
  opt_io_size=<size>     -  (default: 0)
  physical_block_size=<size> - A power of two between 512 B and 2 MiB (default: 0)
  pmrdev=<link<memory-backend>>
  rombar=<uint32>        -  (default: 1)
  romfile=<str>
  serial=<str>
  share-rw=<bool>        -  (default: false)
  smart_critical_warning=<uint8>
  subsys=<link<nvme-subsys>>
  use-intel-id=<bool>    -  (default: false)
  write-cache=<OnOffAuto> - on/off/auto (default: "auto")
  x-pcie-extcap-init=<bool> - on/off (default: true)
  x-pcie-lnksta-dllla=<bool> - on/off (default: true)
  zoned.append_size_limit=<size> -  (default: 131072)

nvme-ns options:
  bootindex=<int32>
  discard_granularity=<size> -  (default: 4294967295)
  drive=<str>            - Node name or ID of a block device to use as a backend
  logical_block_size=<size> - A power of two between 512 B and 2 MiB (default: 0)
  min_io_size=<size>     -  (default: 0)
  nsid=<uint32>          -  (default: 0)
  opt_io_size=<size>     -  (default: 0)
  physical_block_size=<size> - A power of two between 512 B and 2 MiB (default: 0)
  share-rw=<bool>        -  (default: false)
  subsys=<link<nvme-subsys>>
  uuid=<str>             - UUID (aka GUID) or "auto" for random value (default) (default: "auto")
  write-cache=<OnOffAuto> - on/off/auto (default: "auto")
  zoned.cross_read=<bool> -  (default: false)
  zoned.descr_ext_size=<uint32> -  (default: 0)
  zoned.max_active=<uint32> -  (default: 0)
  zoned.max_open=<uint32> -  (default: 0)
  zoned.zone_capacity=<size> -  (default: 0)
  zoned.zone_size=<size> -  (default: 134217728)
  zoned=<bool>           -  (default: false)
```

Create required images and launch QEMU with ZNS SSD:
```shell
qemu-img create -f raw znsssd.img 16777216
qemu-system-x86_64 -name qemucsd -m 4G -cpu Haswell -smp 2 -hda ./arch-qemucsd.qcow2 \
-net user,hostfwd=tcp::7777-:22,hostfwd=tcp::2222-:2000 -net nic \
-drive file=./znsssd.img,id=mynvme,format=raw,if=none \
-device nvme,serial=baz,id=nvme2,zoned.append_size_limit=131072 \
-device nvme-ns,id=ns2,drive=mynvme,nsid=2,logical_block_size=4096,\
physical_block_size=4096,zoned=true,zoned.zone_size=131072,zoned.zone_capacity=131072,\
zoned.max_open=0,zoned.max_active=0,bus=nvme2
```

Week 1 friday demo scripts:
```shell
cat /sys/block/nvme0n1/queue/zoned
cat /sys/block/nvme0n1/queue/chunk_sectors
cat /sys/block/nvme0n1/queue/nr_zones
sudo blkzone report /dev/nvme0n1
sudo nvme zns id-ns /dev/nvme0n1
sudo nvme zns report-zones /dev/nvme0n1
sudo nvme zns open-zone /dev/nvme0n1 -s 0xe40
sudo nvme zns finish-zone /dev/nvme0n1 -s 0xe40
sudo nvme zns report-zones /dev/nvme0n1
sudo nvme zns reset-zone /dev/nvme0n1 -s 0xe40
```