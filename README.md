# c4c

`c4c` is a lightweight C/C++ compiler for NPU and RISC-V-based accelerator systems.

It is built for hardware teams who need a compiler that does exactly what they say —
not one that tries to be smarter than them.

---

## Design Philosophy

**Faithful over clever.**

The compiler's job is to translate your code into machine instructions without
reordering, rescheduling, or second-guessing your intent.

On accelerator hardware, scalar optimization is not the bottleneck.
Semantic fidelity is.

DMA software pipelines, vector instruction sequences, and tensor kernel dispatch
all depend on the compiler preserving the programmer's timing intent exactly.
A compiler that "helps" by reordering instructions breaks these patterns silently
and in ways that are very hard to debug.

`c4c` does not help. It translates.

---

## Core Guarantees

### 1. `inline` is a contract, not a hint

Every function marked `inline` is always inlined at every call site.
No exceptions. No heuristics. No size thresholds.

The call site disappears. The instructions appear there, in order.

This matters because DMA issue points, pipeline synchronization barriers,
and register setup sequences must appear at precise locations in the instruction stream.
A function call overhead or a missed inline breaks the timing model.

```cpp
inline void dma_issue(uint32_t addr, uint32_t size) {
    asm volatile("dma.issue %0, %1" : : "r"(addr), "r"(size));
}

// Guaranteed: no call instruction generated.
// The dma.issue appears exactly here, in the caller's instruction stream.
dma_issue(src_addr, 256);
```

---

### 2. `asm` ordering is a guarantee

Inline assembly sequences are never reordered relative to each other
or relative to surrounding code.

The order you write is the order that executes.

This is the foundation of DMA software pipelining.
A double-buffer pattern depends on issuing the next DMA at exactly the right
point in the compute loop. If the compiler moves the issue instruction,
the overlap collapses and performance degrades silently.

```cpp
// DMA software pipeline — double buffer
// The order of these three asm blocks is load-bearing.
// c4c guarantees they appear in exactly this sequence.

inline void pipeline_iteration(float* sram_a, float* sram_b, float* dram_next) {
    // Issue prefetch for next iteration while computing current
    asm volatile("dma.issue %0, 1024" : : "r"(dram_next));   // (1) prefetch next

    asm volatile("npu.matmul %0, %1" : : "r"(sram_a), "r"(sram_b));  // (2) compute current

    asm volatile("dma.wait");                                  // (3) wait prefetch done
}
// Reordering any of (1)(2)(3) destroys the pipeline.
// c4c does not reorder them.
```

---

### 3. Instruction selection and lowering live outside the compiler

C++ templates and `consteval` are the mechanism for mapping types, shapes,
and hardware variants to specific instructions — entirely in user space,
without touching the compiler core.

**Type-driven opcode generation**

The asm opcode string itself is synthesized at compile time from type information:

```cpp
template<typename T>
consteval const char* vload_opcode() {
    if constexpr (std::is_same_v<T, float>)   return "vload.f32x8";
    if constexpr (std::is_same_v<T, __fp16>)  return "vload.f16x16";
    if constexpr (std::is_same_v<T, int8_t>)  return "vload.i8x8";
}

template<typename T>
inline void vec_load(T* dst, T* src) {
    // Opcode string is a compile-time constant derived from T.
    // No runtime branch. No runtime overhead.
    asm volatile(vload_opcode<T>() " %0, %1" : : "r"(dst), "r"(src));
}

// Usage:
vec_load<float>(dst, src);   // emits: vload.f32x8  dst, src
vec_load<__fp16>(dst, src);  // emits: vload.f16x16 dst, src
```

**Shape-driven kernel selection**

Kernel dispatch based on tensor shape is resolved entirely at compile time
using `consteval` shape queries and `if constexpr` branching:

```cpp
template<typename T>
inline void __sigmoid_short(T* y, T* x) {
    asm volatile(vload_opcode<T>() " v0, %0" : : "r"(x));
    asm volatile("npu.sigmoid.short v1, v0");
    asm volatile(vstore_opcode<T>() " %0, v1" : : "r"(y));
}

template<typename T>
inline void __sigmoid_long(T* y, T* x) {
    asm volatile(vload_opcode<T>() " v0, %0" : : "r"(x));
    asm volatile("npu.sigmoid.long v1, v0");
    asm volatile(vstore_opcode<T>() " %0, v1" : : "r"(y));
}

template<typename T>
Tensor<T> Sigmoid(Tensor<T> x) {
    auto y = new Tensor<T>();
    consteval auto y_shape = get_out_shape(x);

    // Resolved at compile time. Zero runtime branch.
    if constexpr (y_shape[last_dim] < 128) {
        __sigmoid_short<T>(y.data(), x.data());
    } else {
        __sigmoid_long<T>(y.data(), x.data());
    }

    return *y;
}

// Sigmoid<float>  with last_dim=64  → emits __sigmoid_short path, f32 opcodes
// Sigmoid<__fp16> with last_dim=256 → emits __sigmoid_long  path, f16 opcodes
// The compiler sees no branches. It only sees the resolved asm sequence.
```

The result: **your NPU's instruction set lives entirely in C++ headers.**
Changing hardware means changing headers, not rebuilding a compiler.
Adding a new instruction means adding a template specialization and a test.

---

## What c4c Does

- Compiles C/C++ with inline assembly, emitting LLVM IR or direct machine code
- Guarantees `inline` expansion at every call site
- Preserves inline asm ordering — no scheduling across asm boundaries
- Synthesizes asm opcode strings from `consteval` expressions
- Resolves `if constexpr` branches over compile-time shape and type queries
- Implements full compile-time evaluation (`consteval`), including shape inference,
  simple cost modeling, and any arithmetic needed to drive instruction selection
- Supports full C++ template semantics, enabling explicit loop unrolling
  and software pipeline structures written directly in template code —
  the programmer controls the pipeline, not the compiler
- Provides clean extension points for custom NPU/xPU instructions without touching the compiler core
- Stays small enough that an AI agent can read, modify, and maintain it

---

## Template Software Pipeline

The most important use of C++ templates in `c4c` is not type abstraction —
it is **explicit control over pipeline structure**.

A DMA software pipeline has three phases: prologue, steady state, epilogue.
The depth of the pipeline (double buffer = 2, triple buffer = 3) determines
how many iterations are in flight simultaneously.
This structure is written once as a template, parameterized over depth and kernel.

The compute kernel is injected at compile time as a template parameter.
This is compile-time dependency injection: no function pointer, no virtual call,
no indirect branch. After instantiation the compiler sees the exact asm sequence
and inlines it directly into the pipeline body.

```cpp
// pipeline<Depth, T, ComputeFn>
//
// Depth     — pipeline depth: 2 = double buffer, 3 = triple buffer
// T         — element type, propagated to DMA and compute primitives
// ComputeFn — kernel injected at compile time, always inlined
//
// The programmer controls depth, type, and kernel.
// The compiler controls nothing.

template<int Depth, typename T, typename ComputeFn>
inline void pipeline(T** bufs, int N, ComputeFn compute) {

    // Prologue: fill the pipeline
    unroll<Depth>([&](auto i) {
        dma_issue<T>(bufs[i], N);
    });

    // Steady state: issue next, compute current, wait
    for (int i = Depth; i < N; i++) {
        dma_issue<T>(bufs[i], N);
        compute(bufs[i - Depth]);   // ComputeFn resolved at compile time
        dma_wait();
    }

    // Epilogue: drain the pipeline
    unroll<Depth>([&](auto i) {
        compute(bufs[N - Depth + i]);
        dma_wait();
    });
}
```

Usage — inject different kernels into the same pipeline structure:

```cpp
// Sigmoid kernel
auto sigmoid_kernel = [](float* buf) __attribute__((always_inline)) {
    __sigmoid_long<float>(buf, buf);
};

// MatMul kernel
auto matmul_kernel = [](float* buf) __attribute__((always_inline)) {
    __matmul<float>(buf, weight, buf);
};

// Double buffer, float, sigmoid
pipeline<2, float>(bufs, N, sigmoid_kernel);

// Triple buffer, float, matmul
pipeline<3, float>(bufs, N, matmul_kernel);

// These two calls instantiate completely different asm sequences.
// The pipeline structure is shared. The compiler core is not touched.
// The DMA ordering guarantee ensures the prologue / steady state / epilogue
// asm blocks appear in exactly the written order after template expansion.
```

`unroll<N>` expands to N consecutive inline calls at compile time.
Combined with `always_inline` on the kernel lambda, the final output is
a fully unrolled, fully inlined asm sequence with no branches, no calls,
and no compiler-inserted reordering.

The pipeline depth is a compile-time constant.
Changing from double to triple buffer is a one-character edit at the call site.

---

## What c4c Does Not Do

- Aggressive scalar optimization — not the bottleneck on accelerator hardware
- Auto-vectorization — vector patterns are written by the programmer or generated by LLM
- Instruction scheduling — this would silently corrupt DMA software pipeline timing
- Ignore `inline` — if you wrote it, it expands, always
- General-purpose C++ compilation of large runtimes like llama.cpp or PyTorch


---

## Adding a New Instruction

Adding a new NPU vector or tensor instruction requires:

1. Add a `consteval` opcode function or extend an existing one
2. Write the `inline` wrapper function using `asm volatile`
3. Add a regression test that runs on FPGA and checks correctness and cycle count
4. Nothing else — no compiler modification required

Example: adding `vload.bf16x8`

```cpp
// Before: only f32 and f16 supported
template<typename T>
consteval const char* vload_opcode() {
    if constexpr (std::is_same_v<T, float>)   return "vload.f32x8";
    if constexpr (std::is_same_v<T, __fp16>)  return "vload.f16x16";
}

// After: add bf16
template<typename T>
consteval const char* vload_opcode() {
    if constexpr (std::is_same_v<T, float>)    return "vload.f32x8";
    if constexpr (std::is_same_v<T, __fp16>)   return "vload.f16x16";
    if constexpr (std::is_same_v<T, __bfloat16>) return "vload.bf16x8";  // new
}
```

That is the entire change. The compiler is not touched.

---

## Build

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

---

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

---


## License

This project is licensed under the Apache License v2.0 with LLVM Exceptions.
See [LICENSE](LICENSE) for details.

## Contributing

By submitting contributions to this project, you agree that they will be licensed under the project license.
See [CONTRIBUTING.md](CONTRIBUTING.md) for details.
