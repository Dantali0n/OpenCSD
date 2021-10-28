#!/bin/bash

if [ -f "qemu.pid" ]; then
    kill "$(cat qemu.pid)"
    rm nohup.out
    rm qemu.pid
fi