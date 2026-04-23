# `mod.cpp` extraction

## Purpose and current responsibility

This file is the public implementation entrypoint for x86 peephole optimization at the `peephole` namespace boundary. Its only runtime job is to accept whole-assembly text as an owned `std::string` and forward that buffer into the internal pass pipeline.

It does not classify lines, schedule individual rewrites, or own pass ordering logic. Those responsibilities already live below it in `passes::peephole_optimize(...)` and in the shared `peephole.hpp` surface.

## Important APIs and contract surfaces

Essential public surface implemented here:

```cpp
namespace c4c::backend::x86::codegen::peephole {
std::string peephole_optimize(std::string asm_text);
}
```

Actual behavior is delegated to the internal pass-layer surface:

```cpp
namespace c4c::backend::x86::codegen::peephole::passes {
std::string peephole_optimize(std::string asm_text);
}
```

Current contract shape:

- Input is full assembly text, not a structured IR object.
- Ownership is transferred by value and then moved into the pass pipeline.
- Output is rewritten assembly text in the same coarse string form.
- This wrapper promises namespace-level API stability, not optimization semantics.

## Dependency direction and hidden inputs

Dependency direction is one-way:

- `mod.cpp` depends on `peephole.hpp`.
- `mod.cpp` forwards into `passes::peephole_optimize(...)`.
- The pass pipeline then depends on line parsing/classification helpers and per-pass implementations under `passes/`.

Hidden inputs that matter but are not visible in this file:

- The pass ordering and fixed round budget live in `passes/mod.cpp`.
- Line taxonomy, register-family modeling, barrier detection, and textual classifiers come from `peephole.hpp` plus `types.cpp`.
- Behavior is constrained by string-level assembly conventions rather than typed machine-instruction objects.

## Responsibility buckets

What this file actually owns:

- namespace-level public forwarding glue
- the narrow ABI between callers and the internal pass namespace
- preserving move-based transfer into the pass pipeline

What this file does not own:

- parsing assembly into line storage
- line classification and metadata production
- pass sequencing, convergence rules, or cleanup passes
- any x86-specific rewrite decision

## Fast paths, compatibility paths, and overfit pressure

Fast path:

- Minimal wrapper with a single forward call; no local preprocessing or validation.

Compatibility path:

- Keeps a top-level `peephole::peephole_optimize(...)` entry even though real orchestration lives in the nested `passes` namespace. This is an API-compatibility shim more than a behavioral unit.

Overfit pressure:

- Low inside this file itself; it is too small to accumulate testcase-shaped logic.
- Structural pressure exists because the wrapper can mask that callers are coupled to a string-in/string-out legacy pipeline. A rebuild should avoid preserving this boundary only out of habit if a stronger typed seam becomes available.

## Rebuild ownership

This file should own only the stable public entry surface that hands callers into the peephole subsystem.

It should not own pass ordering, text parsing, classification helpers, x86 rewrite rules, or any compatibility special cases beyond the smallest necessary boundary adapter.
