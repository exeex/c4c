# AArch64 Backend Port Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on: `ideas/open/01_backend_lir_adapter_plan.md`

## Goal

Port the AArch64 backend from `ref/claudes-c-compiler/src/backend/` into C++ with behavior and structure kept as close to ref as practical, starting by making the mirrored backend tree compile and then reconnecting it to the existing LIR boundary.

## Why AArch64 First

- it is the highest-priority target in the roadmap
- it is the most natural bring-up target for current development hardware
- it gives the fastest end-to-end proof that the native backend path is viable

## Porting Style

- prefer mechanical translation from Rust to C++
- keep file and mechanism boundaries close to ref
- only diverge where our LIR adapter boundary forces it
- first make the mirrored ref-shaped backend tree include-clean and compilable before chasing behavioral completeness
- reconnect the backend to the existing LIR boundary only after the mirrored code structure is in place
- prefer a stack-first, spill-heavy bring-up over early register-allocation work
- accept obviously non-optimal code shape during bring-up if it keeps correctness and backend boundaries easy to verify

## Primary Ref Surfaces

- `ref/claudes-c-compiler/src/backend/*.rs`
- `ref/claudes-c-compiler/src/backend/elf/`
- `ref/claudes-c-compiler/src/backend/linker_common/`
- `ref/claudes-c-compiler/src/backend/stack_layout/`
- `ref/claudes-c-compiler/src/backend/arm/`

## Immediate Main Axis

This idea should now be driven by two prerequisites before broader feature-by-feature validation:

1. connect includes and declarations so the mirrored AArch64/backend tree can compile as C++
2. reconnect the mirrored backend surfaces to the current LIR adapter and backend entry points

The main progress measure is no longer "one more LLVM IR case passes".

The main progress measure is:

- more of the mirrored ref backend tree is part of the real build
- fewer placeholder boundaries remain between `src/backend/` and the current LIR input
- the AArch64 path moves closer to emitting through the ref-shaped backend pipeline instead of the old LLVM-text fallback path

## Scope

### Bring-up

- make the mirrored backend source tree under `src/backend/` and `src/backend/aarch64/` compile as C++
- add the minimum missing declarations, namespaces, type shims, and include edges needed to wire the mirrored files together
- connect top-level shared backend layers first:
  `generation`,
  `state`,
  `traits`,
  `liveness`,
  `regalloc`,
  `stack_layout`,
  `elf`,
  and `linker_common`
- connect AArch64-local layers next:
  `codegen`,
  `assembler`,
  and `linker`
- reconnect the AArch64 backend entry to the existing backend driver and current LIR-facing boundary
- define the narrowest adapter layer needed where ref consumes SSA/IR structures that do not yet match this repo's LIR structures
- only after those compile-time and interface boundaries exist, begin tightening behavior for prologue/epilogue, memory, ALU, calls, globals, and returns

### Completion

- the mirrored ref-shaped backend tree is no longer just comment scaffolding but a build-participating C++ subsystem
- AArch64 backend entry points are wired through the current backend driver rather than isolated skeleton files
- LIR-to-backend attachment is explicit and narrow enough that further ref-based porting can happen module by module
- the AArch64 backend can emit `.s` for a hello-world-level supported subset
- that emitted `.s` can be assembled into an ELF object by the existing external `llvm-mc` path
- float basics
- calling convention coverage needed by current tests
- stack layout improvements beyond the naive initial model
- a functionally complete stack-first path that can run the intended supported subset even when values are handled conservatively through stack slots across calls
- any AArch64-specific integration work needed to adopt the shared regalloc/cleanup follow-on tracked in `ideas/open/03_backend_regalloc_peephole_port_plan.md`

## Explicit Non-Goals

- x86-64 port
- rv64 port
- built-in assembler
- built-in linker
- polishing every mirrored file to production quality before the include graph is connected
- case-by-case IR-shape chasing when the real blocker is that the mirrored backend tree is not yet wired into the build and LIR boundary

## Suggested Execution Order

1. make the shared mirrored backend files under `src/backend/` compile in isolation enough to be included together
2. make the mirrored AArch64 subtree under `src/backend/aarch64/` compile in isolation enough to be included together
3. reconnect top-level backend entry points, namespaces, and declarations so the AArch64 path is build-reachable
4. reconnect the backend to the current LIR-facing boundary with the smallest adapter/shim layer needed
5. switch the AArch64 backend's main output target from LLVM IR text to AArch64 assembly text
6. make the hello-world-level `.s` output acceptable to the existing external `llvm-mc` path
7. only then resume module-by-module behavioral tightening in ref order:
   `emit`,
   `prologue`,
   `memory`,
   `alu`,
   `comparison`,
   `calls`,
   `globals`,
   `returns`,
   and later follow-ons

## Validation

- more mirrored backend files participate in the real build over time
- the AArch64 backend path can be instantiated from the normal backend driver without falling back to the old LLVM-text route
- the LIR attachment boundary is explicit, narrow, and documented by the code structure
- hello-world-level AArch64 cases emit `.s` instead of backend-specific LLVM IR text
- the emitted `.s` can be assembled into ELF `.o` files through the existing `llvm-mc` path
- compile-time progress is monotonic even before broader runtime behavior is complete

## Follow-On Split Notes

- keep broader local stack-slot/addressing work out of this bring-up idea once it expands beyond the exact `local_temp` single-slot rewrite seam
- track that next local-memory phase in `ideas/open/09_backend_aarch64_local_memory_addressing_plan.md` instead of widening this idea ad hoc
- keep broader global/string address-formation work out of this bring-up idea once it expands beyond exact scalar-symbol loads
- track that next global-addressing phase in `ideas/open/10_backend_aarch64_global_addressing_plan.md` instead of widening this idea ad hoc

## Acceptance Target

For this idea, hello-world-level bring-up is enough.

Acceptance does not require broad runtime or ABI completeness.

Acceptance does require:

- AArch64 backend emits `.s` for the minimal supported slice
- existing external `llvm-mc` can turn that `.s` into an ELF object
- deeper behavioral validation moves to `ideas/open/03_backend_regalloc_peephole_port_plan.md` and later follow-on ideas

## Risk Notes

- host is not the same as the Linux execution target, so the toolchain wrapper must stay boring and deterministic
- avoid burying ABI fixes inside unrelated cleanup patches
- do not get stuck on LLVM IR case-by-case expansion before the mirrored backend tree is even connected
- do not silently invent broad new abstractions just to paper over include or type mismatches; prefer explicit temporary shims

## Good First Patch

Make one thin vertical slice of the mirrored shared backend plus AArch64 backend compile together, hook that slice to the existing LIR-facing backend entry, emit hello-world-level AArch64 `.s`, and prove `llvm-mc` can assemble it into an ELF object.
