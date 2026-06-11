Status: Active
Source Idea Path: ideas/open/173_route7_comparison_condition_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Route 7-Backed Consumer Read

# Current Packet

## Just Finished

Plan Step 1 inventory selected exactly one migration boundary:
`aarch64::codegen::fused_compare_uses_selected_operand(...)` in
`src/backend/mir/aarch64/codegen/comparison.cpp`. This consumer only asks
whether either prepared fused-compare operand has a same-block `Select`
producer before dispatch publishes missing fused-compare operands, so it can
move to Route 7 operand provenance without moving target branch spelling,
fused-compare legality, condition-code selection, emitted registers, hazards,
or final instruction records into BIR.

Nearby prepared oracle/fallback for Step 2: keep
`find_prepared_fused_compare_branch_facts(context, hooks)` as the branch-shape,
operand, target-label, and `can_fuse_with_branch` authority, then compare the
new Route 7 operand-producer answer against the current prepared/BIR semantic
answer from `bir::find_fused_compare_operand_producer_facts(...)`. Existing
prepared equivalence coverage lives in
`verify_prepared_bir_comparison_condition_producer_equivalence()` and
`verify_production_bir_comparison_condition_producer_equivalence()` in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

## Suggested Next

Delegate Step 2 to thread/build the narrow Route 7 comparison-condition index
needed by `fused_compare_uses_selected_operand(...)`, read lhs/rhs operand
producer facts through Route 7 records or route-index validation, and retain
the current prepared/BIR lookup as an oracle or fallback while proving
equivalence.

## Watchouts

- Route 7 facts to prove for the selected consumer: lhs select producer, rhs
  select producer, lhs/rhs producer identity, producer instruction index,
  integer constants for folded operands, immediate operands, and rhs-only
  availability.
- Negative cases to prove: missing operand producer, wrong operand role, wrong
  before-instruction key, duplicate operand producer/reference, non-fused or
  unavailable branch facts, stale owner/block mismatch, and no fallback to
  broad BIR scans when the Route 7 reference is invalid.
- Target subset to protect the consumer behavior:
  `backend_aarch64_instruction_dispatch`, especially
  `selected_global_load_materializes_before_fused_compare_branch()` and
  `rhs_selected_global_load_materializes_before_fused_compare_branch()`.
- Keep target branch spelling, fused-compare legality, condition-code
  selection, hazards, emitted-register state, and final instruction records out
  of BIR.
- Do not contract prepared comparison helpers before the selected consumer has
  Route 7-backed equivalence proof.
- Treat helper renames, expectation rewrites, or named-case-only fixes as
  non-progress.

## Proof

Step 1 was inventory-only; no build or test execution was required, and no
proof logs were created.

Exact narrow proof commands for Step 2 and Step 5:

```bash
cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test
ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure
```
