# BIR Local-Memory GEP And Address Semantics

Status: Closed
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `ideas/closed/597_pointer_address_semantic_model_research.md`
- `ideas/closed/599_pointer_base_plus_offset_selected_authority.md`
- `ideas/closed/600_pointer_value_memory_use_freshness_authority.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: BIR semantic producer
Queue Order: 3
Prerequisites: respect the pointer/address boundary from ideas `597`, `599`, and `600`; do not consume RV64 pointer authority before BIR address facts exist
Estimated Evidence Breadth: `43` local-memory GEP rows, with overlap risk against `27` pointer local-memory consumer rows
Proof Surface: local-memory GEP semantic failures plus nearby pointer/base+offset rows that must remain separated by owner

## Goal

Repair BIR production for local-memory GEP and address formation where the
semantic producer can prove the address shape before prepared or RV64
consumption.

## Why This Exists

The failure map identifies `43` local-memory GEP producer stops. This is a
high-yield ordinary-C lane, but it must not collapse the existing pointer and
address authority model into target-local inference.

## In Scope

- BIR local-memory GEP/address semantics.
- Clear separation between BIR address production and RV64 base+offset
  consumption.
- Proof across multiple GEP-family rows.

## Out Of Scope

- Direct `unsupported_pointer_arithmetic` policy research.
- RV64 local-memory frame-slot consumption.
- Global symbol base+offset, ABI, runtime, expectation, unsupported-marker,
  allowlist, timeout, or accounting changes.

## Acceptance Criteria

- Multiple local-memory GEP rows progress beyond the current BIR producer
  stop.
- Rows that require pointer/address architecture discussion remain rejected or
  classified with that owner.
- The route reuses the established pointer/address model instead of inventing
  a testcase-local exception.

## Closure Summary

Closed as acceptance-satisfied for the ordinary local-memory GEP/address
producer route.

The completed implementation repaired BIR local-memory GEP/address production
in `src/backend/bir/lir_to_bir/memory/addressing.cpp` and
`src/backend/bir/lir_to_bir/memory/local_gep.cpp` by publishing
address/provenance facts for ordinary local-object GEP shapes before
prepared/RV64 handoff. It reused the established pointer/address authority
model from ideas `597`, `599`, and `600` instead of reconstructing target-side
frame addresses or matching testcase names.

Accepted proof:
- `src/pr24851.c` and `src/930614-2.c` now succeed through semantic BIR.
- `src/pr80421.c` progressed beyond the old GEP producer stop to downstream
  `load local-memory semantic family` ownership.
- The delegated 16-test RV64 gcc_torture subset passed `16/16`.
- The refreshed 62-row Step 4 classification recorded `21/62` semantic BIR
  successes, `2/62` downstream load-local stops, `5/62` runtime/intrinsic
  owner stops, and `34/62` remaining GEP-family stops concentrated in
  guard/prerequisite groups.

Remaining rows that still report `gep local-memory semantic family` are not
part of this closed direct-local route without a new lifecycle decision. They
remain separated by owner: pointer/formal provenance, global/static object
GEP, aggregate/member/flexible/alias handling, runtime/string support, and
variadic handling.

## Reviewer Reject Signals

- Reject matching representative case names such as `src/ieee/pr72824-2.c`.
- Reject target-side address inference when BIR did not publish address
  semantics.
- Reject broad pointer arithmetic support hidden inside this idea.
- Reject expectation or unsupported-status downgrades as proof.
- Reject retaining the same local-memory GEP failure behind renamed helpers.
