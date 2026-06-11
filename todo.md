Status: Active
Source Idea Path: ideas/open/206_route7_comparison_provenance_diagnostic_oracle.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The Fail-Closed Route 7 Reader Adapter

# Current Packet

## Just Finished

Step 2 added the fail-closed Route 7 materialized-condition reader adapter in
`find_route7_materialized_condition_producer_identity(...)`.

The adapter now accepts the Route 7 materialized-condition producer only when
the validated comparison record agrees with the current BIR block, condition
value, producing `BinaryInst` pointer/index, opcode/type, and lhs/rhs operand
provenance. Invalid, duplicated, ambiguous, mismatched, or stale Route 7 facts
are rejected as Route 7 and fall through to the existing BIR/prepared fallback.

Focused coverage was added beside the existing materialized compare Route 7
tests for absent records, invalid-reference rejection, condition-name mismatch,
duplicate materialized-condition provenance, and lhs/rhs provenance mismatch.
The tests preserve the valid selected compare shape and the emitted-condition
fallback shape without expected-string weakening. A supervisor-found regression
was repaired so selected global-load compare operands keep their existing
publication fallback instead of reloading stale stack homes.

## Suggested Next

Execute Step 3: wire the selected visible diagnostic/oracle row to consume the
validated Route 7 materialized-condition provenance while keeping branch policy,
condition-code selection, target records, final branch spelling, and printer
authority in the prepared/AArch64 branch machinery.

## Watchouts

- The adapter is intentionally limited to materialized-condition provenance;
  do not broaden Step 3 into fused compare operand readers or Route 7 branch
  condition records.
- Invalid Route 7 operand provenance deliberately falls through to the existing
  BIR/prepared fallback. Some fixtures still emit the condition branch, while
  selected global-load and same-feature materialized compare cases may preserve
  selected compare emission through the older fallback.
- Keep expected-output and branch-policy authority outside Route 7. Step 3
  should not migrate condition-code choice, final branch spelling, hazard
  decisions, or target records into BIR provenance.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch)$' > test_after.log`.

Result: passed. Proof log: `test_after.log`.
