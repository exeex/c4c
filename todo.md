Status: Active
Source Idea Path: ideas/open/206_route7_comparison_provenance_diagnostic_oracle.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Wire The Selected Diagnostic Or Oracle Row

# Current Packet

## Just Finished

Step 3 confirmed the selected visible materialized compare branch row is wired
through the validated Route 7 materialized-condition producer identity while
leaving prepared/AArch64 branch authority intact.

No production change was needed: `lower_materialized_compare_condition_branch(...)`
already consumes `find_route7_materialized_condition_producer_identity(...)` for
the comparison `BinaryInst` and producer instruction index used by operand
publication, while branch targets, final branch spelling, condition suffix
mapping, emitted-register state, hazards, and final assembler row construction
remain outside Route 7.

Focused coverage now pins the selected row as an assembler-backed selected
machine node with validated Route 7 lhs/rhs provenance, exact recomputed
compare lines, prepared true/false labels, and BIR-instruction origin. Invalid,
absent, duplicated, and mismatched Route 7 facts continue to use the existing
BIR/prepared fallback coverage from Step 2.

## Suggested Next

Execute Step 4: prove adapter completeness and route quality for the selected
materialized-condition surface, including nearby same-feature coverage and a
clear statement that Route 7 remains provenance-only.

## Watchouts

- The selected row is still intentionally limited to materialized-condition
  provenance; do not broaden Step 4 into fused compare operand readers or
  Route 7 branch-condition records.
- Invalid Route 7 operand provenance deliberately falls through to the existing
  BIR/prepared fallback. Some fixtures still emit the condition branch, while
  selected global-load and same-feature materialized compare cases may preserve
  selected compare emission through the older fallback.
- Keep condition-code choice, final branch spelling, target records,
  printer/output authority, hazards, emitted-register state, and final
  instruction order outside Route 7.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch)$' > test_after.log`.

Result: passed. Proof log: `test_after.log`.
