# ZCSD

ZCSD is a full stack prototype to execute eBPF programs as if they are
running on a ZNS CSD SSDs. The entire prototype can be run from userspace by
utilizing existing technologies such as SPDK and uBPF. Since consumer ZNS SSDs
are still unavailable, QEMU can be used to create a virtual ZNS SSD. The
programming and interactive steps of individual components is shown below.

![](documentation/resources/images/prototype-landscape.png)

## Getting Started

To get started using ZCSD perform the steps described in the [Setup](#setup)
section, followed by the steps in [Usage Examples](#usage-examples).

### Index

* [Directory structure](#directory-structure)
* [Modules](#modules)
* [Dependencies](#dependencies)
* [Setup](#setup)
* [Running & Debugging](#running-debugging)
    * [Environment](#environment)
    * [Usage Examples](#usage-examples)
    * [Debugging on Host](#debugging-on-host)
    * [Debugging on QEMU](#debugging-on-qemu)
    * [Debugging FUSE](#debugging-fuse)
* [CMake Configuration](#cmake-configuration)
* [Licensing](#licensing)
* [References](#references)
* [Progress Report](#progress-report)
* [Logbook](#logbook)
* 
### Dependencies

This project requires quite some dependencies, the
majority will be compiled by the project itself and installed
into the build directory. Anything that is not automatically
compiled and linked is shown below. Note however, **these dependencies
are already installed on the image used with QEMU**.

**Warning** Meson must be below version 0.60 due to
[a bug in DPDK](https://bugs.dpdk.org/show_bug.cgi?id=836)

* General
    * Linux 5.5 or higher
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

The following dependencies are automatically compiled. Dependencies are preferably
linked statically due to the nature of this project. However, for several dependencies
this is not possible due to various reason. For Boost, it is because the unit test
framework can not be statically linked (easily):

| Dependency                                                         | Version                                                                                                         |
|--------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------|
| [backward](https://github.com/bombela/backward-cpp)                | 1.6                                                                                                             |
| [booost](https://www.boost.org/)                                   | 1.74.0                                                                                                          |
| [bpftool](https://github.com/Netronome/bpf-tool)                   | 5.14                                                                                                            |
| [bpf_load](https://github.com/Netronome/bpf-tool)                  | [5.10](https://elixir.bootlin.com/linux/v5.10.77/source/samples/bpf/bpf_load.h)                                 |
| [dpdk](https://www.dpdk.org/)                                      | 20.11.0                                                                                                         |
| [generic-ebpf](https://github.com/generic-ebpf/generic-ebpf)       | [c9cee73](https://github.com/generic-ebpf/generic-ebpf/commit/c9cee73c73845c9d60aef807b7ee7891987cd6fd)         |
| [libbpf](https://github.com/libbpf/libbpf)                         | 0.5                                                                                                             |
| [libbpf-bootstrap](https://github.com/libbpf/libbpf)               | [67a29e5](https://github.com/libbpf/libbpf-bootstrap/commit/67a29e511cc9d0a570d4d3b9797827f3a08ccdb5)           |
| [linux](https://www.kernel.org/)                                   | 5.14                                                                                                            |
| [spdk](https://github.com/spdk/spdk)                               | 21.07                                                                                                           |
| [isa-l](https://github.com/intel/isa-l)                            | spdk-v2.30.0                                                                                                    |
| [qemu](https://www.qemu.org/)                                      | 6.1.0                                                                                                           |
| [uBPF](https://github.com/iovisor/ubpf)                            | [9eb26b4](https://github.com/iovisor/ubpf/commit/9eb26b4bfdec6cafbf629a056155363f12cec972)                      |

### Setup

Building tools and dependencies is done by simply executing the following commands
from the root directory. For a more complete list of cmake options see the
[Configuration](#configuration) section. The environment file sourced with
`source builddir/qemu-csd/activate` needs to be sourced every time. It
configures essential include and binary paths to be able to run all the
dependencies.

This first section of commands generates targets for host development. Among
these is compiling and downloading an image for QEMU. Many parts of this project
can be developed on the host but some require being developed on the guest. See
the next section for on guest development.

Navigate to the root directory of the project before executing the
following instructions. These instructions will compile the dependencies on the
host, these include a version of QEMU.

```shell script
git submodule update --init
mkdir build
cd build
cmake ..
cmake --build .
# Do not use make -j $(nproc), CMake is not able to solve concurrent dependency chain
cmake .. # this prevents re-compiling dependencies on every next make command
source qemu-csd/activate
# run commands and tools as you please for host based development
deactivate
```

From the root directory execute the following commands for the one time
deployment into the QEMU guest. These command assume the previous section of
commands has successfully been executed. The QEMU guest will automatically start
an SSH server reachable on port 7777. Both the _arch_ and _root_ user can be
used to login. In both cases the password is _arch_ as well. By default the QEMU
script will only bind the guest ports on localhost to reduce security concerns
due to these basic passwords.

```shell script
git bundle create deploy.git HEAD
cd build/qemu-csd
source activate
qemu-img create -f raw znsssd.img 16777216
# By default qemu will use 4 CPU cores and 8GB of memory
./qemu-start.sh
# Wait for QEMU VM to fully boot... (might take some time)
rsync -avz -e "ssh -p 7777" ../../deploy.git arch@localhost:~/
# Type password (arch)
ssh arch@localhost -p 7777
# Type password (arch)
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

Optionally, if the intent is to develop on the guest and commit code, the git
remote  can be updated. In that case it also best to generate an ssh keypair, be
sure to start an ssh-agent as well as this needs to be performed manually on
Arch. The ssh-agent is only valid for as long as the terminal session that
started it. Optionally, it can be included in `.bashrc`.

```shell script
git remote set-url origin git@github.com:Dantali0n/qemu-csd.git
ssh-keygen -t rsa -b 4096
eval $(ssh-agent) # must be done after each login
ssh-add ~/.ssh/NAME_OF_KEY
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

### Running & Debugging

Running and debugging programs is an essential part of development. Often,
barrier to entry and clumsy development procedures can severely hinder
productivity. Qemu-csd comes with a variety of scripts preconfigured to reduce
this initial barrier and enable quick development iterations.

#### Environment:
Within the build folder will be a `qemu-csd/activate` script. This script can be
sourced using any shell `source qemu-csd/activate`. This script configures
environment variables such as `LD_LIBRARY_PATH` while also exposing an essential
sudo alias: `ld-sudo`.

The environment variables ensure any linked libraries can be found for targets
compiled by Cmake. Additionally, `ld-sudo` provides a mechanism to start targets
with  sudo privileges while retaining these environment variables. The
environment can be deactivated at any time by executing `deactivate`.

#### Usage Examples:

TODO: Generate integer data file, describe qemucsd and spdk-native applications,
usage parameters, relevant code segments to write your own BPF program, relevant
code segments to extend the prototype.

#### Debugging on host:
For debugging, several mechanisms are put in place to simplify this process.
Firstly, vscode launch files are created to debug applications even though the
require environmental configuration. Any application can be launched using the
following set of commands:

```shell
source qemu-csd/activate
# For when the target does not require sudo
gdbserver localhost:2222 playground/play-boost-locale
# For when the target requires sudo privileges
ld-sudo gdbserver localhost:2222 playground/play-spdk
```

Note, that when QEMU is running the port _2222_ will be used by QEMU instead.
The launch targets in `.vscode/launch.json` can be easily modified or extended.

When gdbserver is running simply open vscode and select the root folder of
qemu-csd, navigate to the source files of interest and set breakpoints and
select the launch target from the dropdown (top left). The debugging panel in
vscode can be accessed quickly by pressing _ctrl+shift+d_.

Alternative debugging methods such as using gdb TUI or
[gdbgui](https://www.gdbgui.com/) should work but will require more manual
setup.

#### Debugging on QEMU:
Debugging on QEMU is similar but uses different launch targets in vscode. This
target automatically logs-in using SSH and forwards the gdbserver connection.

More native debugging sessions are also supported. Simply login to QEMU and
start the gdbserver manually. On the host connect to this gdbserver and set up
`substitute-path`.

On QEMU:
```shell
# from the root of the project folder.
cd  build
source qemu-csd/activate
ld-sudo gdbserver localhost:2000 playground/play-spdk
```

On host:
```shell
gdb
target remote localhost:2222
set substitute-path /home/arch/qemu-csd/ /path/to/root/of/project
```

More detailed information about development & debugging for this project can be
found in the report.

#### Debugging FUSE:

Debugging FUSE filesystem operations can be done through the compiled filesystem
binaries by adding the `-f` argument. This argument will keep the FUSE
filesystem process in the foreground.

```bash
gdb ./filesystem
b ...
run -f mountpoint
```

### CMake Configuration

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
executed outside of QEMU or not. This is what _IS_DEPLOYED_ facilitates.
Particularly, _IS_DEPLOYED_ prevents the compilation of QEMU from source.