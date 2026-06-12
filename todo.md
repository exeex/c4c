Status: Active
Source Idea Path: ideas/open/215_route7_comparison_provenance_consumer.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Route/Prepared Agreement

# Current Packet

## Just Finished

Step 2 added route/prepared agreement for exactly the selected consumer,
`lower_materialized_compare_condition_branch`.

Changed files:

- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`
- `todo.md`

Implementation completed:

- Split the materialized-condition producer lookup into valid Route 7 evidence
  and explicit generic BIR fallback at the selected consumer.
- Added a prepared materialized-condition producer query local to
  `comparison.cpp`.
- Accepted Route 7 only when it agrees with generic BIR identity and prepared
  materialized-condition authority for the same binary pointer,
  instruction/index, predicate/opcode, compare type, lhs/rhs values and
  operand producer identities, and prepared value name.
- Preserved the existing BIR fallback for absent, invalid,
  duplicate/conflicting, stale, or route/prepared-mismatched Route 7 facts.
- Left branch condition identity, true/false targets, suffix mapping inputs,
  branch policy, fused legality, final ordering, and emitted assembler rows
  under existing prepared/AArch64 ownership.

Proof added:

- `materialized_compare_branch_route7_provenance_matches_bir_identity` now also
  asserts agreement with the prepared materialized-condition producer.
- `materialized_compare_branch_stale_prepared_lookup_uses_bir_fallback` covers
  a selected-consumer route/prepared mismatch shape by attaching stale prepared
  producer lookups while current Route 7/BIR facts remain valid, then verifies
  selected BIR compare fallback (`cmp`/conditional branch, no `cbnz`).

## Suggested Next

Proceed to Step 3: Prove Negative and Fallback Cases for the same selected
consumer. Tighten any remaining consumer-reachable negative proof around
absent, duplicate/conflict, condition-name mismatch, operand-provenance
mismatch, and selected BIR fallback without changing expected strings or
branch-policy ownership.

## Watchouts

- Stale invalid Route 7 materialized-condition references remain helper-level
  because `lower_materialized_compare_condition_branch` builds the Route 7
  index internally from the current block immediately before validation; there
  is no consumer API that can inject a mutated stale-owner index without
  broadening ownership.
- The branch-control negative tests already provide consumer-reachable
  fail-closed/fallback shapes for absent Route 7, duplicate/conflicting
  materialized-condition provenance, condition-name mismatch, lhs/rhs
  provenance mismatch, and stale prepared producer disagreement.
- Do not expand Step 3 into fused compare consumers, Route 8, generic route
  facades, branch policy, suffix/target ownership, machine-printer expected
  strings, or final assembler baseline rewrites.

## Proof

Fresh delegated proof passed and wrote `test_after.log`:

```sh
cmake --build build --target backend_aarch64_branch_control_lowering_test backend_prepared_lookup_helper_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_aarch64_branch_control_lowering|backend_prepared_lookup_helper|backend_aarch64_machine_printer)$' --output-on-failure | tee test_after.log
```

Result: 3/3 tests passed (`backend_aarch64_branch_control_lowering`,
`backend_prepared_lookup_helper`, and `backend_aarch64_machine_printer`).
