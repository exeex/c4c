Status: Active
Source Idea Path: ideas/open/215_route7_comparison_provenance_consumer.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Pin the Selected Consumer

# Current Packet

## Just Finished

Step 1 pinned exactly one Route 7 comparison provenance consumer for active
idea 215: `lower_materialized_compare_condition_branch` in
`src/backend/mir/aarch64/codegen/comparison.cpp`.

Selected consumer:

- Consumer: the materialized compare condition branch lowering path that calls
  `find_route7_materialized_condition_producer_identity` for
  `branch_condition.condition_value`, then emits the selected inline assembler
  `cmp` plus conditional/unconditional branch lines.
- Prepared authority path: `find_prepared_conditional_branch_facts` remains the
  source for the prepared branch condition, target labels, short-circuit/join
  target adjustment, and condition value identity. `condition_home` is still
  resolved through `prepare::find_prepared_value_home_for_bir_value` before the
  Route 7/BIR materialized-compare producer is consumed.
- Generic BIR fallback path: `find_route7_materialized_condition_producer_identity`
  falls back to `find_bir_materialized_condition_producer_identity` when Route 7
  is absent, invalid, stale, duplicate/conflicting, or operand-reference
  validation disagrees. The selected consumer then returns `std::nullopt` if the
  resulting `MaterializedConditionProducerIdentity` is unavailable, preserving
  the emitted-condition path.
- Route 7 evidence/helper path: `bir::route7_build_comparison_condition_index`,
  `bir::route7_find_materialized_condition`,
  `bir::route_index_validate_materialized_condition_reference`,
  `bir::route_index_validate_comparison_operand_reference`,
  `route7_operand_record_matches_reference`, and
  `route7_source_identity_matches_value`.
- Route/prepared agreement point for Step 2: keep the agreement local to this
  selected consumer, either inside the materialized-condition helper or directly
  before `lower_materialized_compare_condition_branch` accepts the available
  identity. The Route 7 comparison record should be accepted only when it agrees
  with the prepared materialized-condition producer answer for the same
  condition value, binary instruction/index, predicate, compare type, lhs, rhs,
  and prepared value name. Disagreement should fall back to the current
  prepared/BIR behavior, not override branch policy.

Coverage map:

- Positive Route 7 evidence: `materialized_compare_branch_route7_provenance_matches_bir_identity`
  verifies Route 7 materialized-condition provenance matches BIR identity and
  the selected branch lowers to the existing `mov`/`add`/`cmp`/`b.le`/`b`
  strings.
- Absent Route 7: `materialized_compare_branch_absent_route7_provenance_uses_emitted_fallback`
  erases the compare and expects emitted-condition fallback (`cbnz`, no `cmp`).
- Duplicate/conflict: `materialized_compare_branch_duplicate_route7_provenance_uses_emitted_fallback`
  creates duplicate `%cond` compare provenance and expects emitted-condition
  fallback.
- Condition mismatch: `materialized_compare_branch_condition_name_mismatch_uses_emitted_fallback`
  changes the binary result name away from the prepared condition and expects
  emitted-condition fallback.
- Operand mismatch: `materialized_compare_branch_lhs_provenance_mismatch_uses_emitted_fallback`
  and `materialized_compare_branch_rhs_provenance_mismatch_uses_emitted_fallback`
  cover lhs/rhs provenance divergence and preserve the selected compare
  fallback (`cmp` plus conditional branch, no `cbnz`).
- Invalid reference: `materialized_compare_branch_invalid_route7_reference_rejected`
  proves stale Route 7 materialized-condition reference validation rejects the
  record, but it is helper-level only today and does not dispatch the selected
  consumer with an injected stale index.
- Helper evidence: `verify_bir_comparison_condition_producer_identity_lookup`
  covers Route 7 materialized-condition/index/facade records plus stale owner,
  wrong role, duplicate reference, missing condition, non-comparison, missing
  producer, and duplicate producer failure modes. `verify_prepared_bir_comparison_condition_producer_equivalence`
  and `verify_production_bir_comparison_condition_producer_equivalence` cover
  prepared/BIR/Route 7 equivalence, including materialized-condition producer
  agreement.

Expected-string and no-change surfaces:

- Branch-control expected-string owners are
  `materialized_compare_branch_does_not_reuse_clobbered_bool_register` and
  `materialized_compare_branch_route7_provenance_matches_bir_identity`; their
  selected assembler rows should stay byte-stable.
- Branch-control fallback helpers own the no-change shape for emitted-condition
  fallback (`cbnz` plus `b`, no `cmp`) and selected compare fallback (`cmp` plus
  `b.<cond>`/`b`, no `cbnz`).
- Machine-printer no-change surface is indirect for this selected consumer:
  `backend_aarch64_machine_printer_test.cpp` owns structured branch/fused
  compare printer strings, while the selected materialized-compare consumer
  emits an assembler record through branch-control lowering. Step 2 should not
  require machine-printer expected-string edits.

Missing proof gaps for Step 2:

- No branch-control test currently mutates prepared materialized-condition
  producer facts to disagree with otherwise valid Route 7 materialized-condition
  facts; Step 2 needs this route/prepared mismatch proof.
- The invalid Route 7 materialized-condition test currently validates a mutated
  index directly, not the selected consumer's internally built index, so Step 2
  should either add a consumer-reachable invalid/fail-closed case or record why
  that invalid shape cannot arise from block-built Route 7 records.
- Positive branch-control proof currently says Route 7 matches BIR identity;
  Step 2 should also assert agreement with prepared materialized-condition
  producer authority, using the prepared lookup/helper surface without moving
  branch policy.

## Suggested Next

Proceed to Step 2 for the single pinned consumer:
`lower_materialized_compare_condition_branch`. Add route/prepared agreement for
the materialized-condition producer only, preserve the existing BIR/emitted
fallback behavior for absent/invalid/duplicate/mismatch cases, and avoid any
branch-policy, suffix, target, fused-legality, final-assembler, expected-string,
Route 8, or generic route-index migration.

## Watchouts

- Keep implementation scoped to the materialized compare condition branch
  consumer; do not also move fused compare operand producer consumers such as
  `fused_compare_uses_selected_operand` or the fused branch lowering paths.
- Route 7 must be advisory evidence for this consumer. Prepared branch facts
  remain authoritative for condition identity, true/false targets, suffix
  selection inputs, branch policy, and final emitted rows.
- Expected-string rewrites, unsupported downgrades, weaker contracts,
  testcase-shaped named-case matching, helper renames, broad route facades, and
  Route 8 work remain blockers.

## Proof

Analysis-only packet; no build or tests were run, and no validation log was
created per the delegated proof contract.

Recommended minimal proof command for later implementation:

```sh
ctest --test-dir build --output-on-failure -R '^(backend_aarch64_branch_control_lowering|backend_prepared_lookup_helper|backend_aarch64_machine_printer)$'
```

The branch-control and prepared-lookup tests are the required proof surface for
the selected consumer. Keep `backend_aarch64_machine_printer` in the subset as
the expected-string no-change guard for adjacent branch/fused-compare printing.
