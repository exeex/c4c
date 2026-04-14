# x86_64 Translated Codegen Integration

Status: Open
Last Updated: 2026-04-13

## Why This Idea

The repository already contains a large translated x86 codegen surface under
`src/backend/x86/codegen/`, including many top-level owner files and the
peephole subtree. The main problem is not translation coverage. The problem is
that much of that translated inventory is still not the real owner of the
active x86 backend path.

Today the tree still leans too heavily on local ownership in:

- `src/backend/x86/codegen/emit.cpp`
- `src/backend/x86/codegen/mod.cpp`
- a set of direct seam files around the current emitter path

This idea exists to make translated x86 codegen real: compiled, reachable, and
truthfully used by the active backend path.

This is separate from shared-BIR matcher cleanup. That work belongs to idea 44.

## Goal

Reduce the gap between translated x86 code already in the tree and the code
that actually owns backend behavior at build and runtime.

Concretely:

- bring translated top-level codegen units into the build in bounded slices
- move ownership out of `emit.cpp` one narrow seam at a time
- keep the translated peephole pipeline as a real reachable stage, not parked
  source inventory
- prove each landed slice with targeted backend evidence

## Non-Goals

- do not turn this into generic x86 backend bring-up
- do not absorb shared-BIR matcher debt from idea 44
- do not silently widen into ABI/runtime/toolchain work unrelated to the chosen
  translated owner slice
- do not claim broad parity with the Rust reference in one activation

## Durable Constraints

- prefer integrating translated files that already exist over inventing new
  local ownership in `emit.cpp`
- move one bounded owner seam at a time
- keep each activation focused on a small translated family or owner file
- treat the Rust/reference layout as the intended ownership map
- require proof that the newly connected translated code is actually used

## Current Durable State

- the translated peephole subtree is already present and should stay treated as
  real backend code, not as speculative inventory
- many translated top-level owner files still exist in-tree without being the
  truthful active owner of the current x86 path
- `variadic.cpp` remains a known high-risk lane and should stay parked unless a
  fresh bounded audit makes it the smallest safe next route
- previous execution showed that blindly promoting the next untranslated owner
  file is not healthy; route selection must stay bounded

## Candidate Surfaces

Translated top-level x86 codegen inventory includes files such as:

- `alu.cpp`
- `asm_emitter.cpp`
- `atomics.cpp`
- `calls.cpp`
- `cast_ops.cpp`
- `comparison.cpp`
- `f128.cpp`
- `float_ops.cpp`
- `globals.cpp`
- `i128_ops.cpp`
- `inline_asm.cpp`
- `intrinsics.cpp`
- `memory.cpp`
- `prologue.cpp`
- `returns.cpp`
- `variadic.cpp`

Future activations should consume bounded slices from this inventory instead of
inventing unrelated x86 work.

## Reactivation Notes

When this idea becomes active again:

- start from the current truthful translated-owner inventory, not from old lane
  history
- choose one bounded translated owner seam
- make the minimum build/header/runtime surface needed for that seam real
- prove the slice with focused backend evidence
- if the next step turns into shared-BIR cleanup or wide variadic/runtime work,
  stop and switch ideas instead of mutating this one
