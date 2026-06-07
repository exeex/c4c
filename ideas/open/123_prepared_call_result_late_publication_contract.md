# 123 Prepared Call Result Late Publication Contract

## Goal

Move the target-neutral call-result late-publication and retargeting decision
into a shared prepared query or fact surface that AArch64 calls lowering can
consume.

## Why This Exists

Idea 118 classified result recording and late publication as
`move-forward-needed`. The current cluster consumes prepared result lanes,
prepared homes, edge-publication source producers, same-block producers, idea
117 current-block publication facts, and live emitted-register state. The
target-neutral question is when a call result source register,
source-in-destination alias, or FPR/VREG store value can be retargeted to an
already-emitted scalar. That decision should be prepared/shared before local
calls lowering is contracted.

## In Scope

- A target-neutral prepared query or fact for call-result late publication and
  source retargeting.
- Coverage for call result source-register recording,
  source-in-destination aliases, and FPR/VREG store-value retargeting to an
  already-emitted scalar.
- Consumption of idea 117 current-block publication facts without rebuilding
  comparison publication routing inside calls lowering.
- AArch64 calls updates only as consumers of the new shared query/fact.
- Dump or route-test visibility showing the call-result publication decision.

## Out Of Scope

- Reopening idea 117 fused-compare, materialized-compare, or current-block
  publication authority.
- Reopening idea 116 prepared producer or same-block producer authority.
- Recomputing destination homes or stack storage covered by idea 93.
- Moving AArch64 FP/f128/GP materialization lines, scratch-register policy,
  q/vector move spelling, or call-boundary machine record emission into shared
  code.
- Broad calls lowering cleanup, comparison cleanup, or dispatch cleanup not
  needed for this prepared query.

## Likely Files

- Shared prepared/BIR prealloc query owners under `src/backend/mir/` or
  `src/backend/prealloc/`
- Existing prepared lookup or printer files that expose call result and
  current-block publication facts
- `src/backend/mir/aarch64/codegen/calls.cpp` as a consumer/proof target
- Existing backend tests covering call result publication, current-block
  publication, FPR/VREG result stores, and AArch64 call-boundary moves

## Owner Boundary

Shared prepared code owns the target-neutral decision describing when a call
result source can publish, alias, or retarget to an already-emitted scalar.
AArch64 calls lowering owns the concrete FP/f128/GP materialization lines,
scratch-register choice, q/vector move spelling, memory-store record emission,
and call-boundary machine records.

## Proof Route

- Characterize the current helpers:
  `record_call_result_source_register`,
  `retarget_fpr_call_result_store_value_to_emitted_scalar`,
  `materialize_call_boundary_source_to_destination`,
  `retarget_call_boundary_source_to_emitted_scalar`,
  `record_call_boundary_destination`, and
  `record_call_boundary_source_in_destination`.
- Add a focused prepared query/fact plus printer or dump visibility for the
  late-publication decision.
- Run a narrow backend/AArch64 subset covering call result source recording,
  source-in-destination aliasing, FPR/VREG store-value retargeting, and
  current-block publication consumption.
- Escalate to broader backend proof if shared publication semantics change
  across comparison or dispatch consumers.

## Acceptance And Completion Criteria

- AArch64 calls lowering consumes a shared call-result late-publication query
  instead of rebuilding current-block publication or same-block producer facts.
- The query covers source-register recording, source-in-destination aliasing,
  and FPR/VREG store-value retargeting, or records a reviewed keep-local
  decision for any excluded case.
- Idea 117 comparison publication facts are consumed rather than duplicated.
- Remaining AArch64 code is limited to target materialization and machine
  record emission once the publication decision is known.
- Proof covers neighboring late-publication routes, not only one named result
  testcase.

## Reviewer Reject Signals

- The route rebuilds current-block publication, comparison publication, or
  same-block producer discovery inside AArch64 calls.
- The route duplicates or reopens idea 117 comparison publication authority or
  idea 116 producer/publication authority.
- The shared query is only a renamed wrapper around the old AArch64 retargeting
  logic with the same failure mode.
- Tests are weakened, supported paths are marked unsupported, or expectations
  are rewritten to avoid late publication.
- The proof covers only one result-retargeting fixture while source-register,
  source-in-destination, FPR/VREG store, and current-block publication
  neighbors remain unexamined.
- Target-specific FP/f128/GP materialization, scratch-register policy, or
  q/vector move spelling moves into shared prepared code.
