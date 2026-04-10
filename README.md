# c4c

`c4c` is a lightweight C/C++ compiler for NPU and RISC-V-based accelerator systems.

## Build

The devcontainer defaults to `Ninja` with `CMAKE_BUILD_PARALLEL_LEVEL=8`, and
the project default build type is `Release`.

```bash
cmake --preset default
cmake --build --preset default
ctest --test-dir build -j8 --output-on-failure
```

---

## Why Another C++ Compiler for xPU

### Cache world vs xPU world

Most compilers assume a cache-driven world: you describe *what* to compute, and hardware decides *when* data arrives. LLVM/MLIR are built around that assumption.

On an xPU, memory is explicit:

* you issue DMA
* you choose when it happens
* you decide when to wait

Overlap is not automatic — it is encoded in instruction order.

> On xPU, timing is part of correctness.

`c4c` preserves DMA order instead of abstracting it away.

---

### The instability problem

There is a second, more practical issue: **optimization behavior is not stable**.

* Scalar passes interfere with vector/tensor scheduling
* Carefully written pipelines get subtly reordered
* Performance collapses with no obvious root cause

On accelerator workloads, this trade-off is misaligned:

* scalar performance is rarely the bottleneck
* vector/tensor throughput is what matters

---

### What actually works (in practice)

After years of building and debugging these systems (including time at AMD,
where approaches like Composable Kernel use C++ with inline asm),
the conclusion is simple:

> The only reliable way to control performance is to make the schedule explicit.

`c4c` does this with a simple combination:

* `asm` → exact control of instruction order
* templates → abstraction + reuse + zero-cost specialization

  * encode software pipeline structure (prologue / steady state / epilogue)
  * express explicit scheduling with a more maintainable syntax
  * enable compile-time specialization over shapes, depths, and kernels

This gives you a well defined pipeline code:

```cpp
template <typename F, std::size_t... Is>
inline void unroll_impl(F&& f, std::index_sequence<Is...>) {
    (f(std::integral_constant<std::size_t, Is>{}), ...);
}

template <std::size_t N, typename F>
inline void unroll(F&& f) {
    unroll_impl(std::forward<F>(f), std::make_index_sequence<N>{});
}

template<typename T>
inline void dma_issue(T* ptr) {
    asm volatile("dma.issue %0" : : "r"(ptr));
}

inline void dma_wait() {
    asm volatile("dma.wait");
}

template<typename T>
inline void square_op(T* ptr) {
    asm volatile("vload v0, %0" : : "r"(ptr));
    asm volatile("vmul v0, v0, v0");
    asm volatile("vstore %0, v0" : : "r"(ptr));
}

// Example: fully template-encoded double-buffer pipeline.
// The schedule is explicit in the template structure itself.
template<int Depth, int N, typename T, typename ComputeFn>
inline void pipeline_static(T** bufs, ComputeFn compute) {
    // Prologue: fill the pipeline
    unroll<Depth>([&](auto i) {
        dma_issue<T>(bufs[i]);
    });

    // Steady state: issue next, compute current, wait
    unroll<N - Depth>([&](auto k) {
        constexpr int i = k + Depth;
        dma_issue<T>(bufs[i]);
        compute(bufs[i - Depth]);
        dma_wait();
    });

    // Epilogue: drain the pipeline
    unroll<Depth>([&](auto i) {
        compute(bufs[N - Depth + i]);
        dma_wait();
    });
}

// Usage: Depth=2, N=4
pipeline_static<2, 4, float>(bufs, square_op<float>);
```

Expected expanded instruction shape:

```asm
// Prologue
DMA.ISSUE bufs[0]
DMA.ISSUE bufs[1]

// Steady state, i = 2
DMA.ISSUE bufs[2]
VLOAD     v0, bufs[0]
VMUL      v0, v0, v0
VSTORE    bufs[0], v0
DMA.WAIT

// Steady state, i = 3
DMA.ISSUE bufs[3]
VLOAD     v0, bufs[1]
VMUL      v0, v0, v0
VSTORE    bufs[1], v0
DMA.WAIT

// Epilogue
VLOAD     v0, bufs[2]
VMUL      v0, v0, v0
VSTORE    bufs[2], v0
DMA.WAIT

VLOAD     v0, bufs[3]
VMUL      v0, v0, v0
VSTORE    bufs[3], v0
DMA.WAIT
```

Here, templates are not just abstraction — **they define the execution structure itself**. The whole pipeline can be encoded directly in templates and expanded at compile time.

This gives you both:

* **assembly-level performance**
* **far lower development cost than MLIR-based stacks**

In practice, this is the only approach we have seen that can consistently push
accelerator performance to the level required to compete with NVIDIA GPUs.

---

## Design Philosophy

**Faithful over clever.**

`c4c` translates your code to instructions without reordering or second-guessing.

On accelerators, performance depends on preserving timing intent (DMA pipelines, vector sequences, kernel dispatch). “Helpful” reordering can silently break it.

`c4c` keeps what you wrote—exactly as written.

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
    asm volatile("dma.issue %0, 1024" : : "r"(dram_next));        // (1) prefetch next

    asm volatile("npu.matmul %0, %1" : : "r"(sram_a), "r"(sram_b));  // (2) compute current

    asm volatile("dma.wait");                                      // (3) wait prefetch done
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

## FAQ

**Q: Adding a new instruction requires changing a header. Is that the only way?**

No. Besides extending the `consteval` opcode headers directly, `c4c` provides
an assembler customization interface so users can register new instructions
without touching the header library at all. The header path is for teams who
want compile-time type-safe wrappers. The assembler interface is for teams who
want to wire in new opcodes at a lower level or drive emission programmatically.

---

**Q: What does the register allocator handle?**

The RA covers scalar registers and basic allocation for Scalable Vector instructions.
If you do not want calling convention interference — for example, if your kernel
issues a precise sequence of vector ops and you cannot afford spills or unexpected
saves — write the register assignments by hand in inline asm.
The RA will not fight you. It will leave your asm blocks alone.

---

**Q: Who manages tensor register lifetime?**

You do. `c4c` assumes tensor operands have no general-purpose register backing.
The compiler does not track tensor register liveness, does not insert
spill/reload sequences, and does not enforce any ownership model on tensor state.
This is intentional. Tensor register management is part of the kernel contract,
not the compiler contract.

---

**Q: How much of the C++ standard library is supported?**

`c4c` aims for broad `std` coverage. Standard library compatibility is one of
the core internal test metrics — if something in `std` breaks, that is a
compiler bug, not a user problem.

That said, for tensor data containers we recommend writing your own.
`std::vector` and similar containers carry allocator assumptions and
exception machinery that do not belong in hot kernel paths.
A purpose-built `Tensor<T>` with explicit lifetime gives you full control
over memory layout, alignment, and SRAM placement.

---

**Q: How large is the compiler binary? Can it run on bare metal?**

Target size is 5–10 MB. A bare-metal deployment path is planned, which makes
`c4c` viable as a JIT compiler on embedded systems and a natural fit as a
custom backend for `torch.compile`.

---

**Q: Can I mix ****************************************************************`c4c`****************************************************************-compiled code with GCC or Clang output?**

Yes. `c4c` emits standard `.o` object files that link cleanly with output
from GCC, Clang, or any other toolchain that follows the target ABI.

The recommended split: use GCC or Clang for scalar-heavy utility code,
and `c4c` for kernels where inline asm ordering and `inline` guarantees matter.
Link the two together as normal. The linker does not know or care which
compiler produced which object.

---

**Q: Will there be heterogeneous compilation support?**

Yes, planned. The target model is similar to NVCC's `__host__` / `__device__`
split, extended to cover multi-core dispatch with a `<<<>>>` -style syntax
for RISC-V control cores, DMA engines, and custom accelerator datapaths.
The exact interface is still being designed.

---

**Q: Is there a language extension planned beyond standard C++?**

Yes, under discussion. The working name is `.c4` — a C++ superset that may
draw from several directions:

* **Circle C++** — compile-time metaprogramming patterns that go beyond what
  standard `consteval` can express cleanly
* **Full reflection** — enough to replace TableGen entirely, so instruction
  definitions can be written in the language itself
* **Rust-style ownership** — borrowing the ownership and lifetime model to
  make tensor and buffer aliasing explicit and checkable at compile time

None of this is finalized. Proposals are welcome.

---

**Q: Why not use MLIR?**

xPU and CPU/GPU differ fundamentally in how memory moves.

On a CPU or GPU, memory access goes through a cache hierarchy. The hardware
decides when to fetch, when to evict, and when data is ready. The programmer
describes what they want; the hardware figures out how to get it. LLVM and MLIR
were designed in this world. Their memory model assumes cache — a transparent
layer that makes "load from address X" work without the programmer thinking
about timing.

On an xPU, there is no cache. Memory moves through DMA. DMA is explicit:
the programmer issues a descriptor, specifies source, destination, and size,
and later explicitly waits for completion. The overlap between DMA and compute
is not automatic — it is the programmer's responsibility to manage issue timing,
channel occupancy, and completion detection.

MLIR was built for the CPU/GPU world and carries that assumption throughout.
`memref.copy` is a cache-world abstraction: "move this memory, somehow, at some point."
The `async` dialect adds dependency tokens, not timing guarantees.
Neither concept maps to what DMA actually requires.

Adapting MLIR to an xPU means fighting this assumption at every layer of the
lowering pipeline. Timing intent expressed at the source level does not survive
to the backend that generates DMA descriptors, because none of the intermediate
representations were designed to carry it. The backend scheduler must reconstruct
the pipeline structure from a dependency graph that no longer contains the
information needed to do this correctly.

C++ with inline asm does not have this problem. The DMA issue is an instruction.
The wait is an instruction. Their positions in the code are their positions in
the instruction stream. There is no intermediate representation to lose the timing.

For a detailed worked example, see [MLIR_vs_cpp.md](MLIR_vs_cpp.md).

---

## License

This project is licensed under the Apache License v2.0 with LLVM Exceptions.
See [LICENSE](LICENSE) for details.

## Contributing

By submitting contributions to this project, you agree that they will be licensed under the project license.
See [CONTRIBUTING.md](CONTRIBUTING.md) for details.
