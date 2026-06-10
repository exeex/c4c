# 167 Route 1 producer and constant oracle thinning

## Goal

Migrate one selected producer/constant/materialization consumer group from
prepared same-block producer helpers to Route 1 BIR producer records, then
contract only the cache/API surface that no longer has public consumers.

## Source

This idea comes from Phase C:

- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- Route 1 row under `## Step 4 Follow-Up Ideas And Proof Routes`

## Scope

- Same-block scalar producer queries.
- Immediate integer constant queries.
- Same-block value-materialization query/cache use.
- Source-producer lookup cache exposure through `PreparedFunctionLookups`.
- One bounded consumer class at a time.

## Out Of Scope

- Homes, registers, storage, emitted-register state, operand views, frame slots,
  final instruction order, spill/reload behavior, and publication hooks.
- Deleting prepared producer/constant helpers before the selected consumer no
  longer needs them as an oracle.
- Broad AArch64 materialization rewrites.

## Acceptance Criteria

- The selected consumer reads `Route1ProducerIndex` or a thin BIR-backed facade.
- Prepared answers remain available as an oracle until equivalence is proven.
- Negative cases such as no producer, no constant, and non-materializable values
  are covered.
- Any cache/API contraction is limited to surfaces no longer publicly consumed.

## Proof Route

Use focused MIR/shared-producer or AArch64 materialization equivalence tests
that compare BIR and prepared answers, then run the matching backend prepared
lookup subset plus the relevant target lowering subset.

## Reviewer Reject Signals

- Broad consumer switching without route-level equivalence proof.
- Importing target storage, register, home, or final-emission policy into BIR.
- Deleting prepared helpers while higher-route consumers still use them as
  migration oracles.

## Completion Note

Closed after migrating the selected AArch64 GP scalar value-publication
materialization path in
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` to use
Route 1-backed same-block scalar producer and integer-constant queries for the
selected semantic facts.

The selected consumer now reads through
`mir::find_same_block_scalar_producer(...)` and
`mir::evaluate_same_block_integer_constant(...)`. Prepared answers remain
available for residual target-policy and non-selected consumers. Step 4 found
no prepared producer, constant, cache, or API surface safe to contract yet
because remaining AArch64 and prealloc consumers still publicly require those
surfaces, so no contraction was performed.

Close proof used the canonical narrow backend logs:
`test_before.log` and `test_after.log` both covered
`backend_prepared_lookup_helper` and `backend_aarch64_instruction_dispatch`.
The c4c regression guard passed in non-decreasing mode with 2 passed, 0 failed
before and after.
