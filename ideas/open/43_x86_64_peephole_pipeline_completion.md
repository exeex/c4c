# x86_64 Peephole Pipeline Completion

Status: Open
Last Updated: 2026-04-10

## Why This Idea

The x86 backend already contains a translated peephole optimizer under
`src/backend/x86/codegen/peephole/`, and the reference implementation under
`ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/` has largely
been mirrored file-for-file.

But the pipeline is not actually complete on the current tree:

- the pass orchestration entry point in
  `src/backend/x86/codegen/peephole/passes/mod.cpp` is still a stub that
  returns the original assembly text
- the x86 codegen path does not currently appear to route emitted assembly
  through the peephole optimizer
- the existence of translated pass files therefore overstates actual backend
  capability

This is not the same problem as shared-BIR legacy matcher cleanup. That work
belongs to idea 44. This idea is specifically about finishing the x86 assembly
peephole pipeline so the translated pass set becomes a real backend stage.

## Source Context

- `ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/mod.rs`
- `src/backend/x86/codegen/peephole/mod.cpp`
- `src/backend/x86/codegen/peephole/passes/mod.cpp`
- `src/backend/x86/codegen/emit.cpp`

## Current State

The reference Rust pipeline runs several phases:

1. iterative cheap local passes
2. one-shot global cleanup passes
3. post-global local cleanup
4. loop trampoline cleanup
5. tail-call optimization
6. dead never-read store cleanup
7. unused callee-save elimination
8. frame compaction

On the current C++ tree:

- many individual pass files exist
- helper and type infrastructure also exists
- the top-level orchestration function is still stubbed
- integration into the x86 emission path is not wired up

So the immediate gap is orchestration plus integration, not raw file presence.

## Goal

Make the translated x86 peephole pipeline real and reachable:

- implement pass orchestration in `peephole/passes/mod.cpp`
- verify the translated pass interfaces and line-store assumptions are usable
- integrate `peephole_optimize(...)` into the x86 assembly emission path
- add focused regressions that prove the pass pipeline actually transforms
  assembly in owned cases

## Non-Goals

- do not treat peephole completion as a substitute for shared-BIR cleanup
- do not widen this into a generic x86 emitter rewrite
- do not silently absorb unrelated ABI/runtime or shared-BIR matcher debt
- do not promise full parity with the Rust reference in one slice if the
  infrastructure still needs bounded enablement work

## Working Model

- treat the Rust reference as the intended pipeline order
- prefer enabling existing translated passes over inventing new ones
- validate the narrowest pass orchestration slice first
- prove integration with targeted asm-shape tests before broad-suite claims

## Proposed Plan

### Step 1: Audit the translated peephole surface

Goal: determine which translated pass files are ready, which depend on missing
infrastructure, and what the minimum viable orchestration path is.

Completion check:

- the pass inventory is explicit
- the first activatable pass subset is identified

### Step 2: Implement orchestration in `passes/mod.cpp`

Goal: replace the current stub with a real pass pipeline matching the reference
order as closely as the current C++ infrastructure allows.

Completion check:

- `passes::peephole_optimize(...)` no longer returns input unchanged by default
- targeted pass-level tests prove at least local and global pass rounds run

### Step 3: Wire peephole optimization into x86 emission

Goal: make the optimizer part of the actual x86 output path.

Completion check:

- emitted x86 assembly flows through `peephole_optimize(...)`
- targeted backend tests demonstrate observable optimized output

### Step 4: Expand enabled pass coverage carefully

Goal: turn on additional translated passes only when their assumptions hold on
the current emitter output.

Completion check:

- the enabled pass set is documented
- any still-disabled or partial pass has an explicit reason
