Status: Active
Source Idea Path: ideas/open/216_route7_comparison_oracle_row.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Pin the Helper-Oracle Row

# Current Packet

## Just Finished

Step 1 pinned one helper-oracle row for active idea 216.

Selected row: materialized-condition row in `verify_prepared_bir_comparison_condition_producer_equivalence`, specifically the success assertion around `prepare::find_prepared_materialized_condition_producer(...)` for condition `%cond` at `block.insts.size()` in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

Row type: materialized-condition.

Current helper-oracle baseline:

- Prepared oracle: `prepare::find_prepared_materialized_condition_producer(...)` returns a `PreparedMaterializedConditionProducer` only for a named condition whose same-block scalar producer is a binary instruction. The selected row currently expects `binary == condition_binary`, `instruction_index == 3`, and `condition_value_name == names.value_names.find("%cond")`.
- Existing BIR helper parity: `bir::find_materialized_condition_producer_identity(block, %cond, block.insts.size())` is available and matches the prepared binary, instruction index, condition name, and populated lhs/rhs producer facts.
- Existing Route 7 evidence: `bir::route7_comparison_instruction_record(&block, 3)` is available and already matches the prepared binary, instruction index, and condition value name. The stronger route-native query/reference candidates are `route7_build_comparison_condition_index(block)`, `route7_find_materialized_condition(index, block, %cond, block.insts.size())`, and `route_index_validate_materialized_condition_reference(route_index_reference_facade(index), block, %cond, block.insts.size())`.

Baseline/proof matrix for this one row:

- Positive: covered by the selected helper-oracle row plus existing Route 7 materialized-condition records in `verify_bir_comparison_condition_producer_identity_lookup`; later implementation should assert the indexed Route 7 materialized-condition reference beside the prepared producer, not replace branch-policy behavior.
- Absent route: branch-control coverage exists in `materialized_compare_branch_absent_route7_provenance_uses_emitted_fallback`; helper-level absent evidence exists through missing `%missing.cond` materialized-condition reference returning `MissingRecord` / `NoMatch`.
- Invalid reference: branch-control coverage exists in `materialized_compare_branch_invalid_route7_reference_rejected`, expecting stale Route 7 materialized-condition references to reject with `StaleOwner`.
- Duplicate/conflict: branch-control coverage exists in `materialized_compare_branch_duplicate_route7_provenance_uses_emitted_fallback`; helper-level duplicate reference coverage exists for Route 7 duplicate materialized/operand references.
- Mismatch: branch-control coverage exists in `materialized_compare_branch_condition_name_mismatch_uses_emitted_fallback`, `materialized_compare_branch_stale_prepared_lookup_uses_bir_fallback`, `materialized_compare_branch_lhs_provenance_mismatch_uses_emitted_fallback`, and `materialized_compare_branch_rhs_provenance_mismatch_uses_emitted_fallback`.
- Unfused fallback: keep the existing fused helper negative in `verify_prepared_bir_comparison_condition_producer_equivalence` where `PreparedBranchConditionKind::MaterializedBool` makes `find_prepared_fused_compare_operand_producer_facts(...)` return no value; do not broaden this packet into fused-compare row replacement.
- Prepared fallback: AArch64 materialized-condition lowering still consults prepared materialized-condition producer identity and falls back to BIR/emitted-condition behavior unless Route 7 and prepared agree. Keep prepared as the authority for the fallback contract.
- Expected-string owners: branch-control owns materialized compare emitted lines, especially `mov w9, w13`, `mov w10, #16`, `add x9, x9, x10`, `cmp w9, #0`, `b.le ...`, and final unconditional branch in the materialized Route 7 tests. Machine-printer owns fused-compare branch spelling only; include it as a stability guard, not as selected row scope.
- Wrapper/stability proof candidates: `PreparedFunctionLookups` attachment and AArch64 block/function lowering contexts in branch-control tests are the wrapper stability surface. No wrapper contraction is implied by this row.
- Missing proof gaps for Step 2: the selected helper-oracle row does not yet assert the indexed Route 7 materialized-condition query/reference directly beside the prepared row. Step 2 should add that one route-native assertion while preserving the prepared producer assertion and all expected strings.

## Suggested Next

Step 2 should add the minimal route-native assertion for the selected materialized-condition row in `backend_prepared_lookup_helper_test.cpp`: build the Route 7 comparison-condition index for the existing fixture, validate/find `%cond` as a materialized condition at `block.insts.size()`, and assert it agrees with the existing prepared producer. Do not touch fused-compare rows, branch policy, wrappers, printer/debug formatting, or expected strings.

## Watchouts

- Keep this to one helper-oracle row.
- The selected row is materialized-condition only: `verify_prepared_bir_comparison_condition_producer_equivalence` / `find_prepared_materialized_condition_producer(...)` for `%cond`.
- Preserve prepared fallback for positive, absent-route, invalid-reference, duplicate/conflict, mismatch, unfused fallback, and prepared fallback behavior.
- Do not migrate branch policy, printer/debug formatting, wrapper behavior, final assembler behavior, Route 8 return-chain work, or generic route-index facades.
- Treat expected-string rewrites, helper renames, baseline refreshes, unsupported downgrades, weaker contracts, and testcase-shaped oracle handling as blockers.
- Recommended minimal implementation proof later: `cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_machine_printer)$' --output-on-failure`.

## Proof

Analysis-only packet per supervisor proof contract; no build or CTest run, and no validation log was created or modified.
