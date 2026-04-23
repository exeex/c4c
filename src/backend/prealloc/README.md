# Prealloc Contract

`src/backend/prealloc/` owns the target-sensitive preparation phase between
shared semantic BIR and target-local backend emission.

Its job is not just to publish raw facts. It should publish authoritative plans
that a backend can consume without rebuilding the same decisions locally.

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

The new plan structs are present as contract scaffolding. Producer logic still
needs to be filled in so `prealloc.cpp`, `stack_layout.cpp`, and `regalloc.cpp`
populate them from the existing prepared facts.
