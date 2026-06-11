# 189 Phase E cross-target route-view interface reuse

## Goal

Reuse proven AArch64 route-view interfaces at x86 or riscv prepared wrapper
boundaries.

## Why This Exists

Phase D kept x86 and riscv at interface level. Cross-target reuse should happen
only after an AArch64 route-view rung proves a semantic interface, so target
wrappers can consume the same view without inventing target-specific BIR
adapters first.

Source: `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`.

## In Scope

- One x86 `ConsumedPlans` or riscv prepared emission/wrapper boundary.
- A route view already proven by an earlier AArch64 Phase E migration idea.
- Interface-level compile, route/debug, or wrapper tests that show the target
  can consume the shared semantic interface.

## Out Of Scope

- Designing a new x86-only or riscv-only BIR adapter before AArch64 proof.
- Moving x86/riscv instruction selection, target formatting, frame layout,
  register allocation, call ABI policy, or emission records into BIR.
- Broad prepared aggregate contraction.

## Acceptance Criteria

- The target wrapper consumes a route view previously proven by an AArch64
  migration slice.
- Target-local policy remains owned by x86 or riscv lowering.
- Tests prove interface reuse without weakening route-debug, wrapper, or target
  emission coverage.

## Reviewer Reject Signals

- A target-specific adapter is invented before the shared AArch64 route view is
  proven.
- Target policy is moved into BIR to make reuse look complete.
- The slice claims prepared aggregate contraction from interface threading.
- Tests are limited to compile-only when behavior or route/debug coverage is
  required by the touched boundary.
- The implementation duplicates `PreparedFunctionLookups` under a BIR-owned
  name.
