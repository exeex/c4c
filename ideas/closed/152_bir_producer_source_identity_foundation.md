# 152 BIR producer/source identity foundation

## Goal

Add the target-neutral BIR producer/source identity foundation needed before
later BIR normalization routes can move out of prepared/prealloc ownership.

## Why This Exists

Phase A identified same-block producer/materialization as the first dependency
for select-chain, memory, publication, edge, call, and comparison routes. The
current prepared query surface already answers this relationship, but BIR lacks
a durable owner/query for the semantic subset.

Input artifact: `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
sections `## BIR-Normalization Candidates`, `## Dependency Order`, and
`## Follow-Up Idea Payloads`.

## In Scope

- Define a BIR-owned producer-kind vocabulary that does not depend on
  publication/prealloc enum authority.
- Add a per-function or query-cache relationship over block label,
  before-instruction index, value/name/type, same-block producer node,
  immediate constants, and materialization availability.
- Bridge or compare against `find_prepared_same_block_scalar_producer` and
  `evaluate_prepared_same_block_integer_constant`.
- Switch at most a narrow low-risk consumer after query equivalence is proven.

## Out Of Scope

- Register names, homes, operand views, emitted-register availability,
  spill/reload behavior, storage encoding, final instruction order, or
  publication-owned enum authority in the BIR schema.
- Broad MIR emission rewrites before BIR/prepared query equivalence exists.

## Acceptance Criteria

- BIR queries return the same semantic subset as the prepared same-block and
  constant queries on representative scalar, cast, binary, load, select, and
  constant cases.
- The old prepared query remains available as the comparison oracle.
- Any consumer switch is narrow and limited to identity reads already proven
  equivalent.

## Proof Route

Capture matching `test_before.log` and `test_after.log` for the smallest
backend/codegen subset that exercises same-block scalar materialization. Add
targeted dumps or assertions showing BIR/prepared query equivalence for each
supported producer kind.

## Reviewer Reject Signals

- Imports `PreparedValueHome`, register-view state, storage state, or final
  emission state into the BIR relationship.
- Reuses publication enum naming as the BIR authority instead of defining a
  target-neutral BIR vocabulary.
- Switches broad MIR emission before query equivalence is proven.
- Proves only one named case while nearby same-block producer kinds remain
  unexamined.

## Closure Note

Closed 2026-06-10.

Implemented the target-neutral BIR producer identity foundation in the MIR
query surface. The completed API includes BIR-owned same-block producer
identity request/value/record types, a semantic producer-kind vocabulary for
binary, cast, select, load-local, and load-global producers, and
`find_same_block_producer_identity(...)` as the identity owner. The scalar
producer and integer-constant query helpers now build on that identity surface
while preserving same-block boundary and type checks.

Prepared query APIs remain available as the comparison oracle. Equivalence
coverage proves prepared/BIR agreement for same-block scalar producers across
binary, cast, load-local, load-global, select, and product cases, plus integer
constant agreement for immediate and binary constant cases. Fail-closed cases
cover future producers, missing facts, mismatched value types, mismatched
prepared facts, and nonconstant scalar producers.

Exactly one narrow consumer switched during this idea:
`find_prepared_same_block_select_producer` now reads select producer identity
through the BIR identity query. The switch is limited to materializable select
records with matching BIR instruction, instruction boundary, value name, and
value type. Broad MIR emission, scalar producer recursion, constant evaluation
ownership, storage/register/publication ownership, and final instruction-order
behavior remain outside this closed idea.

Rejected target-specific state stayed out of the BIR relationship: no register
homes, storage state, publication-owned enum authority, emitted-register
availability, spill/reload behavior, operand views, frame-slot layout, or final
instruction order were imported into the BIR schema.

Final matched proof logs:

- `test_before.log`: 3/3 passed for `backend_x86_shared_producer_query`,
  `backend_prepared_lookup_helper`, and
  `backend_aarch64_instruction_dispatch`.
- `test_after.log`: 3/3 passed for `backend_x86_shared_producer_query`,
  `backend_prepared_lookup_helper`, and
  `backend_aarch64_instruction_dispatch`.
- `c4c-regression-guard --allow-non-decreasing-passed`: PASS for the matched
  logs.

Remaining route boundaries are intentionally left to the already-open follow-up
ideas `153` through `158`: select-chain direct/global identity, memory access
identity, block-entry publication identity, CFG edge publication identity, call
boundary source facts, and comparison condition producer identity.
