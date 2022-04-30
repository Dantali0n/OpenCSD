# Python environment

Contains various files to generate plots or analyze results. Running any script
in this directory is likely to require the libraries specified in
_requirements.txt_. The easiest solution to install these dependencies is
by setting up the python folder as virtualenv.

```shell
virtualenv -p python3 python
rm python/.gitignore
cd python
source bin/activate
pip3 install -r requirements.txt
```

## Index

1. [graphs](graphs)
1. [csd-*](#csd-kernel-files)
2. [regular-*](#regular-files)

### CSD kernel files

The files starting with `csd-` perform CSD kernel operations in tandem with
their application on the FluffleFS filesystem.

These files have the following prerequisites:

1. FluffleFS must be mounted under the `.test/` directory relative from the
   current path (not the path of python files themselves).
2. The kernel objects registered and launched by the python file must exist
   on the mounted FluffleFS filesystem. The `bin` folder inside the project
   build directory `./build/qemu-csd/bin` will contain these.
3. A test file with (often) arbitrary test data must exist under `/test` from
   the FluffleFS mount point. Due to the simplicity of kernels the size must be
   sector aligned.

### Regular files

Some python files are used to recreate behavior of CSD kernels without
offloading. Typically, these start with `regular-`.
