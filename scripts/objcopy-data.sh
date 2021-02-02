#!/bin/bash

#DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
#cd $DIR

# Statically convert OpenCL sourcecode into a data object. This can be used to compile OpenCL
# sourcecode into a binary as string.
objcopy --input binary --output elf64-x86-64 --binary-architecture i386 data.cl data.o