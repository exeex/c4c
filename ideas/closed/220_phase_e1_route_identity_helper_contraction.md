# 220 Phase E1 route identity helper contraction

## Goal

Move one selected Route 1 through Route 7 source, publication, join, call, or
comparison identity helper or reader to route/prepared agreement, while
retaining prepared ownership for target policy, fallback/oracle behavior,
wrappers, diagnostics, printer/debug rows, and expected strings.

## Why This Exists

Phase E1 triage classified the route
source/publication/join/call/comparison identity helper family as ready to
draft one implementation idea. The family has named semantic ownership in
Route 1 through Route 7 source, producer, select-chain, direct-global,
memory/source, publication, edge/join, call-use, fused-compare, and
materialized-condition provenance annotations under route/prepared agreement.
It also has clear retained prepared surfaces for address formation, value
homes, storage, move scheduling, call ABI, publication construction, branch
spelling, fused legality, wrappers, printer/debug rows, helper oracles,
failure-mode fallback, and target-policy-sensitive output.

This idea exists to create one bounded implementation runbook from that
accepted readiness classification. It must choose one route family and one
consumer or identity helper before code changes.

## In Scope

- Select exactly one route family and one selected consumer or identity helper
  from the Phase E1 route identity candidate surface.
- Candidate surfaces include:
  `find_prepared_same_block_scalar_producer(...)`,
  `evaluate_prepared_same_block_integer_constant(...)`,
  `find_prepared_direct_global_select_chain_dependency(...)`,
  `find_prepared_store_source_direct_global_select_chain_dependency(...)`,
  `PreparedMemoryAccessLookups`, `find_prepared_memory_access(...)`,
  `find_prepared_global_load_access(...)`,
  `find_prepared_same_block_global_load_access(...)`,
  `PreparedEdgePublicationLookups`,
  `PreparedEdgePublicationSourceProducerLookups`,
  `find_prepared_current_block_entry_publication(...)`,
  `find_indexed_prepared_edge_publications(...)`,
  `find_indexed_prepared_edge_publication_source_producer(...)`,
  `PreparedCallPlanLookups`,
  `find_prepared_call_argument_source_producer_materialization(...)`,
  `find_prepared_call_result_late_publication(...)`,
  `find_prepared_fused_compare_operand_producer_facts(...)`, or
  `find_prepared_materialized_condition_producer(...)`.
- Use route/prepared agreement for the selected identity fact only.
- Keep prepared fallback for absent, invalid, ambiguous/conflict, mismatch, and
  policy-sensitive cases.
- Preserve output, wrapper, printer/debug, helper-oracle, and expected-string
  behavior.

## Out Of Scope

- Broad migration of Route 1 through Route 7, generic route-index facades, or
  aggregate route view replacement.
- Address formation, value homes, storage, move scheduling, call ABI,
  publication construction, branch spelling, fused legality, wrapper behavior,
  printer/debug rows, helper oracles, failure-mode fallback, target policy, or
  emitted output.
- `PreparedFunctionLookups`, `PreparedBirModule`, call-plan, edge-publication,
  memory-access, move-bundle, publication, comparison, wrapper, or prepared API
  retirement.
- Row-scoped diagnostic/oracle replacement unless the row is only the proof
  harness for the selected semantic identity reader.
- Liveness, intrinsic metadata, aggregate forwarding, E2, E3, E4, Route 8,
  draft 155, or E5 work.
- Baseline refreshes, unsupported downgrades, helper renames, timeout masking,
  or expectation rewrites as proof.

## Proof Requirements

- Positive agreement proof: route/prepared agreement supplies the same
  selected identity fact to the chosen consumer or helper.
- Absent proof: missing route fact keeps prepared fallback behavior.
- Invalid proof: wrong or unusable route references fail closed to prepared
  behavior.
- Ambiguous/conflict proof: duplicate, conflicting, or ambiguous route facts do
  not override prepared behavior.
- Mismatch proof: route/prepared disagreement preserves the prepared result.
- Policy-sensitive fallback proof: target policy and prepared-owned mechanics
  for the selected route family remain unchanged.
- Output proof: printer/debug rows, wrapper output, helper-oracle behavior, and
  expected strings remain byte-stable.
- Nearby same-feature proof: at least one adjacent same-feature case remains
  covered so the slice is not shaped only around one testcase.

## Reviewer Reject Signals

- Reject broad Route 1 through Route 7 migration, generic route facade work, or
  prepared aggregate retirement.
- Reject moving address formation, value homes, storage, move scheduling, call
  ABI, publication construction, branch spelling, fused legality, target
  policy, wrappers, printer/debug rows, helper oracles, or expected strings
  into route authority.
- Reject testcase-shaped handling of one route case without absent, invalid,
  ambiguous/conflict, mismatch, policy fallback, and output proof.
- Reject baseline refreshes, helper renames, unsupported downgrades, timeout
  masking, or expected-string rewrites as evidence.
- Reject preserving prepared-only behavior behind a route-named facade without
  actually replacing one duplicate semantic identity read.

## Closure Note

Closed after the selected-helper runbook completed for Route 7 comparison
provenance: AArch64
`find_prepared_fused_compare_operand_producer_facts(...)`, consumed by
`lower_prepared_conditional_branch_terminator(...)`.

The completed slice added a Route 7/prepared agreement gate in
`src/backend/mir/aarch64/codegen/comparison.cpp` while preserving prepared
fallback and output authority for non-agreement paths. It did not open broad
Route 1 through Route 7 migration, generic route facades, aggregate route view
replacement, prepared aggregate retirement, target-policy movement, wrapper or
printer/debug changes, helper-oracle changes, or expected-string rewrites.

Closure proof recorded in `todo.md`:

- focused selected-helper proof passed 2/2;
- close-time regression guard passed with `--allow-non-decreasing-passed`,
  before 2/2 and after 2/2 with no new failures;
- broader `^backend_` subset passed 180/180 after the code slice.

Residual route identity helper candidates named by this idea remain outside
this closed one-helper source scope. Any further route identity helper
contraction should be opened or activated as a separate lifecycle initiative
rather than silently extending this completed runbook.
