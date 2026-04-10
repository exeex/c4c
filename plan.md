# x86_64 Translated Codegen Integration

Status: Active
Source Idea: ideas/open/43_x86_64_peephole_pipeline_completion.md
Activated from: user-prioritized switch on 2026-04-10

## Purpose

Replace the current x86 `emit.cpp` legacy ownership surface with the translated
reference-owned emitter shape so the real backend path stops depending on
piecemeal `emit.cpp` matcher transfer work.

## Goal

Turn the translated x86 emitter shape into the active owner of x86 emission in
one directed migration:

- use `ref/claudes-c-compiler/src/backend/x86/codegen/emit.rs` as the target
  ownership map
- delete the remaining legacy `emit.cpp` matcher/dispatcher surface instead of
  continuing bounded sibling-seam extraction
- connect or port the required translated support units so the migrated emitter
  can compile and run as the real backend path
- keep validation centered on backend regressions for x86 emission behavior

## Core Rules

- Prefer porting or wiring the translated reference-owned implementation over
  adding fresh `emit.cpp`-local logic.
- Keep this plan scoped to x86 translated-code integration and emitter-owner
  replacement.
- Do not silently absorb shared-BIR cleanup from idea 44 into this runbook.
- Treat backend regression coverage as the primary validation surface for this
  plan, but defer full regression-guard monotonic gating until after the
  `emit.cpp` owner switch is complete; frontend LLVM IR checks are out of scope
  unless a backend edit changes shared frontend/codegen plumbing.
- Land changes in coherent backend-owned clusters, not one testcase-shaped
  matcher at a time.

## Current Scope

- translated top-level x86 codegen units required to support the reference
  emitter shape:
  `alu.cpp`, `asm_emitter.cpp`, `atomics.cpp`, `calls.cpp`, `cast_ops.cpp`,
  `comparison.cpp`, `f128.cpp`, `float_ops.cpp`, `globals.cpp`,
  `i128_ops.cpp`, `inline_asm.cpp`, `intrinsics.cpp`, `memory.cpp`,
  `prologue.cpp`, `returns.cpp`, `variadic.cpp`
- translated peephole subtree needed by the active emission path:
  `peephole/mod.cpp`, `peephole/types.cpp`, and the translated pass files under
  `peephole/passes/*.cpp`
- the active x86 emitter entry surface in `src/backend/x86/codegen/emit.cpp`
- `src/backend/x86/codegen/x86_codegen.hpp`
- `CMakeLists.txt`
- reference ownership files under
  `ref/claudes-c-compiler/src/backend/x86/codegen/`

## Non-Goals

- no shared-BIR legacy matcher cleanup beyond what must be removed from x86
  `emit.cpp` to complete the emitter-owner switch
- no unrelated frontend/parser/LLVM IR work
- no testcase-by-testcase legacy seam expansion to keep `emit.cpp` alive longer
- no unrelated runtime or non-x86 backend refactors

## Execution Phases

### Step 1: Audit the reference emitter dependency surface

Goal: identify the exact support units, state fields, and build wiring needed
to make `ref/.../emit.rs` the active ownership map in C++.

Completion check:

- the translated-unit dependency inventory is explicit
- the required `X86Codegen` state/method gaps are recorded
- the first compileable migration cluster is chosen

### Step 2: Make the translated emitter dependencies compile and link

Goal: bring the required translated support units into the build and expose the
minimum supporting `X86Codegen` surface needed by the reference emitter shape.

Completion check:

- the required dependency cluster compiles in the active build
- backend tests prove the wired path is reachable

### Step 3: Replace `emit.cpp` with the translated emitter owner

Goal: port the active C++ emitter entry and dispatch around the reference
`emit.rs` design, deleting the legacy matcher/dispatcher body instead of
continuing seam extraction.

Completion check:

- `src/backend/x86/codegen/emit.cpp` no longer owns the legacy direct matcher
  surface
- the translated emitter-owner path is the real x86 backend path
- focused backend regressions cover the switched path sufficiently to unblock
  the cutover

### Step 4: Reconnect peephole and residual translated integration

Goal: restore any remaining translated emission stages required by the new
owner path, including the peephole pipeline and residual helper units.

Completion check:

- emitted x86 assembly flows through the intended translated integration path
- remaining disconnected translated units are explicit residual debt, not
  hidden legacy ownership in `emit.cpp`
- the backend full-suite regression guard is rerun only after the ownership
  cutover settles
