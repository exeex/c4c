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
