# 161 BIR memory access identity annotation schema

## Goal

Prototype BIR annotations for memory/access semantic identity established by
Phase A Route 3.

## Why This Exists

Phase A proved BIR-owned direct memory access, same-block global-load identity,
and same-block load-local source identity. Phase B classified memory access
identity as instruction annotations with value annotations for result or stored
value links, while rejecting target layout and addressing policy.

## In Scope

- Define instruction annotations for load/store/address node identity,
  address-space, volatile flag, semantic base kind, and source
  local/global/pointer/string identity where BIR can represent it.
- Define value annotations for load result values, store source values, and
  same-block load-local stored-value source links.
- Keep prepared memory/access queries as migration oracles for positive and
  negative equivalence.
- Preserve cheap same-block global-load and load-local lookup through typed
  indexes that reference annotation payloads.

## Out Of Scope

- `PreparedAddress` wholesale schema, frame slot ids, concrete byte offsets,
  size/align layout, base-plus-offset legality, TLS register or relocation
  spelling, GOT/direct/page-low policy, target addressing-mode choice, and
  AArch64 memory operand legality.

## Acceptance Criteria

- BIR-owned annotations can answer the semantic access identity questions
  proven by Phase A without exposing layout or target-addressing decisions.
- Prepared memory/access answers remain available as oracle checks until
  consumers are safely switched.
- Target-facing address materialization remains owned by prealloc/codegen.

## Reviewer Reject Signals

- Reject if annotations contain frame slot ids, byte offsets, size/align,
  relocation spelling, TLS register details, addressing-mode legality, or
  AArch64 operand formation.
- Reject if a `PreparedAddress` or `PreparedMemoryAccess` record is copied as
  the schema shape instead of field-level semantic identity.
- Reject if same-block global-load success is proven by named-case matching
  while volatile, address-space, local-source, and no-source cases are left
  unsupported or unexamined.
- Reject if expectation rewrites weaken memory/access contracts rather than
  proving BIR and prepared answers equivalent.
- Reject broad changes to frame layout or target addressing code as part of
  this schema prototype.
