# Prealloc Contract

`src/backend/prealloc/` owns the target-sensitive preparation phase between
shared semantic BIR and target-local backend emission.

Its job is not just to publish raw facts. It should publish authoritative plans
that a backend can consume without rebuilding the same decisions locally.

## Markdown Policy

This directory should contain no root-level markdown shards other than this
`README.md`. Historical comparison notes from the Rust reference path should
be reconciled into live C++ ownership, durable ideas, focused tests, or this
README, then deleted.

The `stack_layout/` subdirectory has its own `README.md` because it is an
active implementation submodule, not a loose markdown shard. Do not recreate
root comparison files such as `liveness_comparison.md`,
`regalloc_comparison.md`, or `stack_layout_comparison.md`.

## Published Authority

`prealloc/` is the owner of:

- register allocation policy and final assigned homes
- stack-slot placement and frame sizing
- dynamic stack regions such as VLA / `alloca(n)` lifetimes
- caller-saved versus callee-saved policy
- call argument and result placement
- target-facing move-resolution plans around calls and returns

The minimum published target-facing contracts are:

- `PreparedFramePlan`
- `PreparedDynamicStackPlan`
- `PreparedCallPlans`
- `PreparedStoragePlans`

Within those contracts, register-facing facts should use explicit banks:

- `gpr`: integer and pointer ABI/register homes
- `fpr`: floating-point ABI/register homes
- `vreg`: vector or SIMD register homes

These live in [prealloc.hpp](/workspaces/c4c/src/backend/prealloc/prealloc.hpp:1) beside the
older lower-level facts such as `PreparedStackLayout`, `PreparedRegalloc`, and
`PreparedValueLocations`.

## Backend Boundary

Target backends such as x86 should:

- trust these plans as authority
- map banked logical register names to target spellings
- render prologue/epilogue/call/operand syntax from the published plans
- honor published dynamic stack operations for VLA and stack-save/restore

Target backends should not:

- redo regalloc
- choose caller-saved versus callee-saved policy again
- repack stack slots
- rediscover VLA or dynamic-alloca lifetime rules from raw call names
- reconstruct call placement from raw BIR shape when a prepared call plan is
  available

## Current Status

The stack-layout implementation is live. `BirPreAlloc::run()` calls
`run_stack_layout()`, and that phase is implemented by
`stack_layout/coordinator.cpp` plus `stack_layout/*.cpp`. It collects stack
objects, applies alloca/copy/regalloc/inline-asm hints, assigns frame slots,
publishes `PreparedStackLayout`, and seeds prepared addressing facts.

The broader prealloc schema and producers are still too concentrated in
`prealloc.hpp`, `prealloc.cpp`, `regalloc.cpp`, and `prepared_printer.cpp`.
The planned cleanup direction is tracked by the open prealloc decomposition
ideas: split schema headers first, then producer/fact publishers, then
regalloc implementation, then the prepared printer.

Prealloc-local Rust reference files have been removed from this directory.
Active implementation guidance lives in the C++ sources, focused tests, this
README, and open decomposition ideas. Historical Rust material, when needed
for archaeology, belongs under `ref/claudes-c-compiler/` rather than beside
the active prealloc code.
