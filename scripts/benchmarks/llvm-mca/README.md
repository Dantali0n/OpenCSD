# LLVM-MCA cycle accurate static analysis

Scripts used to perform LLVM-MCA static analysis and make cycle
accurate predictions on the required CPU cycles for various hardware platforms.

```shell
clang shannon-entropy-512k.c -O3 --target=arm -mcpu=cortex-a9 -mtune=cortex-a9 -mfloat-abi=soft -S -o - | llvm-mca -march=arm -mcpu=cortex-a9
```

## Hardware platforms from associated literature

- BlockNDP - Dual core, Cortex A9 @ 800-1000 Mhz
- 