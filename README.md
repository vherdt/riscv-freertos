# FreeRTOS for RISC-V Virtual Prototype (VP)

This is a port of FreeRTOS to RISC-V with a focus of running it on the RISC-V VP (see https://github.com/vherdt/riscv-vp). This port is based on a fork from https://github.com/illustris/FreeRTOS-RISCV. To build the demo applications the RISC-V GNU toolchain is required. Build instructions for the toolchain are also available at https://github.com/vherdt/riscv-vp.

## Contributors
The original port to priv spec 1.7 was contributed by [Technolution](https://interactive.freertos.org/hc/en-us/community/posts/210030246-32-bit-and-64-bit-RISC-V-using-GCC)

Update to priv spec 1.9: [illustris](https://github.com/illustris)

Update to priv spec 1.9.1: [Abhinaya Agrawal](https://bitbucket.org/casl/freertos-riscv-v191/src)

Bug fixes: [Julio Gago](https://github.com/julio-gago-metempsy)

Update to priv spec 1.10: [sherrbc1](https://github.com/sherrbc1)

Update to support external interrupt sources and demo applications for the RISC-V VP: [vherdt](https://github.com/vherdt)

## Build

You can edit `main()` in `main.c` (Demo/standard-demo/main.c, also check the other demos) to add your FreeRTOS task definitions and set up the scheduler.

To build a FreeRTOS demo (requires the RISC-V GNU toolchain to be available in PATH),

```bash
cd Demo/standard-demo
make
```

## Run
```bash
riscv-vp riscv-main.elf --memory-start=2147483648
```

