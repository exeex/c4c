# c4c

`c4c` is a C/C++ compiler project for AI infrastructure and general RISC-V-based xPU systems.

It is designed for a future where accelerator software is not limited to a fixed vendor toolchain. The goal is to make C/C++ a practical systems language for heterogeneous AI hardware, while keeping the path from language feature to hardware feature short enough for real hardware teams to use.

## Vision

`c4c` is being built to support the compilation of modern C++ AI projects such as `llama.cpp`, `torch`, and other performance-critical runtime stacks.

Beyond compiling conventional C/C++ code, `c4c` aims to provide an integrated heterogeneous programming and linking model similar in spirit to `__host__` and `__device__` in CUDA, but designed for open and customizable RISC-V-based accelerator systems.

This means one toolchain can eventually describe, compile, and link software across:

- host CPU code
- RISC-V control processors
- DMA engines
- custom ASIC datapaths
- vendor-specific xPU execution units

## Why c4c

Today, adding custom instructions or hardware-specific behavior to a compiler stack often requires touching multiple LLVM layers and maintaining `.td` files across a complex backend flow. That approach is powerful, but it is also expensive, slow to iterate, and difficult for many hardware vendors to adopt.

`c4c` takes a different direction.

By strengthening `consteval` as a core mechanism, `c4c` is intended to give hardware vendors a much faster way to introduce custom instructions and hardware-specific programming models, without having to modify the compiler core for every architectural experiment.

The long-term idea is simple:

- vendors should be able to express hardware behavior in C++-level constructs
- custom instruction definitions should be easier to prototype and evolve
- architecture bring-up should not require deep LLVM backend expertise

## Custom xPU Architecture Definition

`c4c` is intended to let you define your own heterogeneous xPU architecture on top of arbitrary combinations of:

- RISC-V cores
- DMA subsystems
- fixed-function ASIC blocks
- custom accelerators and instruction extensions

The key goal is that you can customize this hardware programming model without touching the compiler kernel itself.

In other words, `c4c` is not only a compiler project. It is an attempt to make compiler-driven hardware customization accessible enough that vendors can explore and productize new xPU designs with much lower integration cost.

## Build

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

Notes:

- `clang` is optional for building `c4cll` itself.
- `clang` is required for runtime or execute-style tests and for turning emitted LLVM IR into a runnable binary.

## Testing

Run the default core suite:

```bash
cd build
ctest --output-on-failure
```

Or use the convenience target:

```bash
cd build
cmake --build . --target ctest_core
```
