# AArch64 Memory Store-Retargeting Owner

## Goal

Split the AArch64 memory-local store-retargeting helpers into a narrow owner
that rewrites memory records around prepared pointer, frame-slot, and emitted
scalar facts without publishing new shared dispatch, BIR, storage, or register
authority.

## Why This Exists

The memory owner subresponsibility audit found a concrete local boundary around
pointer store-value/address retargeting, local-address store-value rewrites,
and stack-layout application to memory records. The route survived historical
comparison because it is a private memory-record rewrite responsibility, not a
generic helper publication or a dispatch/publication relocation.

## In Scope

- Create a focused AArch64 memory-local owner for store-retargeting helpers
  currently clustered in `memory.cpp` and `memory.hpp`.
- Own pointer store-value/address retargeting, retargeting to materialized
  pointer addresses, retargeting to emitted scalar registers, local-address
  store-value rewrites, and stack-layout application to selected memory
  records.
- Preserve existing memory lowering entry points and keep the helper surface
  private or narrowly exported to existing AArch64 memory consumers.
- Prove equivalent behavior for pointer-address stores, emitted-scalar store
  reuse, local-address store rewrites, and stack-layout application.

## Out Of Scope

- Creating shared BIR, dispatch-publication, storage-plan, or register helper
  authority from these retargeting helpers.
- Moving store-local/store-global publication planning or edge-copy
  publication helpers into this owner.
- Reopening dispatch publication relocation, edge-copy contraction, or
  prepared-wrapper contraction from ideas 80, 81, or 84.
- Weakening diagnostics or unsupported-path contracts to make the split easier.
- Broad memory-record builder extraction beyond the store-retargeting rewrite
  boundary.

## Acceptance And Proof Expectations

- The implementation introduces a narrow AArch64 memory-local owner whose API
  is limited to store-retargeting and stack-layout rewrite responsibilities.
- Focused proof covers pointer store-value/address retargeting,
  materialized-address retargeting, emitted-scalar store reuse, local-address
  store-value rewrite, and stack-layout application to memory records.
- Build proof includes at least the backend targets affected by AArch64 memory
  lowering.
- Acceptance-sized slices should use matching before/after regression logs if
  helper movement affects public headers or shared AArch64 memory call sites.

## Reviewer Reject Signals

- The route publishes generic storage, register, diagnostic, or dispatch helper
  authority instead of keeping retargeting memory-local.
- Store publication, edge-copy, or prepared-wrapper work is folded into this
  idea despite the historical boundaries from ideas 80, 81, and 84.
- The implementation changes selected memory-record semantics, diagnostics, or
  unsupported-path behavior while claiming the slice is only a local-owner
  split.
- Test expectations are downgraded, or the proof covers only one named
  pointer-store case while nearby emitted-scalar, materialized-address,
  local-address, and stack-layout rewrite paths are unexamined.
- The old retargeting failure mode remains behind a new abstraction name.
