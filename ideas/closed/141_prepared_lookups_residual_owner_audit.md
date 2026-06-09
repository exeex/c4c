# 141 Prepared lookups residual owner audit

## Goal

Audit the remaining public and implementation surface in
`src/backend/prealloc/prepared_lookups.hpp` and
`src/backend/prealloc/prepared_lookups.cpp` after ideas 136-140 moved the first
round of domain-owned query APIs out of the broad prepared lookup facade.

This idea was analysis-only. It did not directly rewrite prealloc or AArch64
codegen. It produced focused follow-up ideas only where a remaining
prepared-lookup group has a clearer domain owner or exposes target-facing
routing shape that should be separated from reusable prepared facts.

## Closure Result

Status: Closed

The runbook completed all five audit steps. The close payload classified every
remaining residual group, mapped definitions and construction wiring, mapped
AArch64 and prealloc consumer pressure, identified no-new-idea groups, and
recommended the long-term role of `PreparedFunctionLookups`.

Close-time regression guard passed on a matched temporary backend before/after
pair:

- command scope: `ctest --test-dir build -j --output-on-failure -R backend`
- before: 400 passed, 0 failed, total 400
- after: 400 passed, 0 failed, total 400
- mode: `--allow-non-decreasing-passed` because closure is lifecycle-only

## Durable Audit Payload

The audit concluded that `PreparedFunctionLookups` and
`make_prepared_function_lookups` should remain the central one-pass aggregate
and field-projection boundary. Follow-up work should shrink domain-specific
declarations around that aggregate; it should not delete the aggregate or
replace reusable prepared facts with local BIR rescans, predecessor rescans, or
name matching.

Groups kept under `prepared_lookups.*`:

- `PreparedFunctionLookups` and `make_prepared_function_lookups`.
- `PreparedReturnChainLookups` and return-chain query helpers, because no
  concrete return-chain owner comparable to `calls.hpp`, `value_locations.hpp`,
  `publication_plans.hpp`, or `select_chain_lookups.hpp` was proven.

Groups that already have acceptable ownership:

- same-width and aggregate stack-source publication helpers already live in
  `publication_plans.hpp/.cpp`.
- broad includes through module aggregation are not owner-move evidence by
  themselves.

Concrete follow-up ideas created from the audit payload:

- `ideas/open/142_value_home_move_bundle_lookup_ownership.md`
- `ideas/open/143_stack_layout_id_lookup_helpers_owner.md`
- `ideas/open/144_source_producer_same_block_materialization_owner.md`
- `ideas/open/145_current_block_join_fact_routing_split.md`
- `ideas/open/146_call_argument_materialization_call_owner.md`
- `ideas/open/147_comparison_prealloc_fact_owner.md`
- `ideas/open/148_same_block_load_local_stored_value_owner.md`
- `ideas/open/149_residual_prepared_lookup_include_cleanup.md`

## Completion Evidence

The close payload was assembled from committed execution packets:

- `70b9e2162` for Step 1 inventory.
- `77dc273bf` for Step 2 definition and construction map.
- `40134c67b` for Step 3 consumer map.
- `b411c3588` for Step 4 classification.
- `56dcee9eb` for Step 5 closure payload.

## Reviewer Reject Signals

Historical closure should be questioned if a later route claims this audit
authorized:

- deleting reusable prepared facts and replacing them with target-local
  rediscovery;
- moving AArch64 register spelling, scratch policy, hazard policy, or final
  instruction emission into shared prealloc code;
- treating line count or include count alone as owner-move evidence;
- reopening ideas 137-140 without a concrete residual dependency from this
  audit;
- moving return-chain lookups without first naming a concrete return-chain
  owner.
