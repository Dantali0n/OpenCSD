[![pipeline status](https://gitlab.dantalion.nl:4443/root/airglow/badges/master/pipeline.svg)](https://gitlab.dantalion.nl:4443/root/airglow/commits/master)
[![coverage report](https://gitlab.dantalion.nl:4443/root/airglow/badges/master/coverage.svg)](https://gitlab.dantalion.nl:4443/root/airglow/commits/master)
# AirGlow
### Vulkan based toy game engine

Shenanigans for past time.

### Project goals

* Implement an entity component system (ECS) similar to the one in the
bennybox Youtube series.
* Implement a sparse voxel octree ray-tracing system
* Experiment with task based multi-threading using a system like
[taskflow](https://github.com/taskflow/taskflow)
* Experiment with heterogeneous computing by utilizing integrated
graphics either via Vulkan or OpenCL.
* Experiment with memory-pool based allocators.
* Maintain a module based design approach that allows to dynamically
include or exclude modules or switch between them at compile time. 

### Directory structure

* airglow - project source files
* cmake - small cmake snippets to enable various features
* dependencies - 
* docs - doxygen generated source code documentation
* documentation - engine documentation written in LaTeX
* lib - support library with functions and definitions
* playground - small toy examples or other experiments
* [python](python/README.md) - python scripts to aid in visualization or measurements
* tests - unit tests and possibly integration tests

### Modules

| Module     | Optional | Task                                               |
|------------|----------|----------------------------------------------------|
| arguments  | Yes      | Parse commandline arguments to relevant components |
| entry      | No       | Entry function from main                           |

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
    
The following dependencies are automatically compiled:

| Dependency                                                         | Version     |
|--------------------------------------------------------------------|-------------|
| [booost](https://www.boost.org/)                                   | 1.74.0      |
| [glfw](https://www.glfw.org/)                                      | 3.3.2       |
| [vulkan-headers](https://github.com/KhronosGroup/Vulkan-Headers)   | 1.2.157     |
| [vulkan-loader](https://github.com/KhronosGroup/Vulkan-Loader)     | 1.2.156     |
| [opencl-headers](https://github.com/KhronosGroup/OpenCL-Headers)   | v2020.06.16 |
| [opencl-loader](https://github.com/KhronosGroup/OpenCL-ICD-Loader) | v2020.06.16 |

#### Setup

```shell script
git submodule update --init --recursive
mkdir build
cd build
cmake ..
cmake --build .
source airglow/activate.sh
# run commands and tools as you please
deactivate
```

```shell script
virtualenv -p python3 python
cd python
source bin/activate
pip install -r requirements.txt
```

#### Licensing

Some files are licensed under a variety of different licenses please see
specific source files for licensing details.

#### References

* [Emebedding source files into binaries](https://www.linuxjournal.com/content/embedding-file-executable-aka-hello-world-version-5967)
* [OpenCL, SyCL and SPIR-V progress 2016](https://www.youtube.com/watch?v=TYp1d6yzHUQ)
* [Using SDL or GLFW](https://gamedev.stackexchange.com/questions/32595/freeglut-vs-sdl-vs-glfw)
* [Nvidia sparse voxel octrees](https://www.nvidia.com/docs/IO/88972/nvr-2010-001.pdf)

#### Ideas

Some ideas and possible methods / strategies.

**Actor accessing strategies:**
Accessing actors needs to happen quickly and often. Locality and cache coherence
are critical properties for actor accessing algorithms.

* Actors could be placed into a map that can be accessed in `Olog(n)` time.
* Actors can be placed into a vector being accessed in `Olog(1)` time. This
makes modifications (creating / deleting) actors very expensive.
* Tiered actor system combing a vector and map. Each unique actor type / class
will have a unique ID placed into the vector. Subsequently, each instance
is placed into the map at that vector location.
* Chunk bases system where actors are placed based in a grid on their current
location. Requires moving the place of actors in the grid if they move.


#### Snippets

```bash
 /opt/rocm/bin/rocprof --obj-tracking on --hsa-trace binary
```

Extract gettext messages from source code.
```bash
xgettext --keyword=translate:1,1t --keyword=translate:1c,2,2t       \
         --keyword=translate:1,2,3t --keyword=translate:1c,2,3,4t   \
         --keyword=gettext:1 --keyword=pgettext:1c,2                \
         --keyword=ngettext:1,2 --keyword=npgettext:1c,2,3          \
         *.cxx
```