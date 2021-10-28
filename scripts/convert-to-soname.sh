#!/bin/bash

# convert all .so files in the current directory to their SONAME
for f in ./*.so; do
    NAME=$(objdump -p "$f" | grep SONAME | awk '{print $2}')
    if [ ! -f "$NAME" ]; then
        ln -s "$f" "$NAME"
    fi
done
