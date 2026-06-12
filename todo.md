Status: Active
Source Idea Path: ideas/open/228_phase_e3_fused_compare_operand_producer_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Public Fallback And Nearby Same-Feature Cases

# Current Packet

## Just Finished

Completed Step 2 - Validate Existing Agreement-Gated Route 7 Evidence.

The private agreement-reader implementation path is present in
`src/backend/mir/aarch64/codegen/comparison.cpp`. AST-backed checks found:
- anonymous-namespace
  `find_prepared_fused_compare_operand_producer_facts(...)` at line 424 calls
  `detail::read_agreeing_route7_fused_compare_operand_producer_facts(...)`;
- the private reader at line 405 builds a Route 7 comparison-condition index,
  reads `bir::route7_find_fused_compare_operand_producer_facts(...)`, and then
  delegates to
  `route7_fused_compare_operand_producer_facts_if_agree_with_prepared(...)`;
- `lower_prepared_conditional_branch_terminator(...)` is the public caller and
  passes the helper result into
  `make_prepared_conditional_branch_record_impl(...)`.

Agreement behavior is already gated on both converted sides matching prepared
facts. `route7_fused_compare_operand_producer_facts_if_agree_with_prepared(...)`
rejects unavailable Route 7 facts, converts Route 7 operands through
`route7_comparison_operand_producer_to_prepared(...)`, rejects failed
conversions, and returns converted facts only when both
`prepared_fused_compare_operand_producer_matches(converted.lhs, prepared.lhs)`
and `prepared_fused_compare_operand_producer_matches(converted.rhs,
prepared.rhs)` are true. If the private reader does not return agreed facts,
the public helper returns the original prepared facts. No semantic gap was
found, so no implementation or test behavior was changed.

Existing private proof covers these fallback cases in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`:
- absent Route 7 evidence through an empty/absent block route;
- unavailable Route 7 consumer facts;
- invalid producer references;
- duplicate/conflicting Route 7 branch-condition records;
- mismatched converted operands;
- unfused/unavailable Route 7 facts;
- partial non-agreement where one side is missing;
- wrong-key, wrong-role, missing-producer, and stale-reference Route 7
  consumer-boundary failures before the private agreement check.

Public-boundary Step 3 proof gaps remain proof gaps, not implementation gaps:
- no focused AArch64 public helper/caller test currently mutates the selected
  fused-compare operand-producer Route 7 failure modes and proves the emitted
  compare operand row is still the prepared fallback row;
- prepared-only fallback is covered indirectly by prepared aggregate and broad
  branch-control tests, but not by a focused public-boundary test that withholds
  usable Route 7 evidence and asserts unchanged prepared compare operands;
- nearby same-feature tests exist for immediate/rhs-only/production-prealloc
  BIR facts, branch-control output, folded constants, stack-homed constants,
  loop-header fused branches, and selected global-load lhs/rhs operands, but
  Step 3 still needs focused public-boundary proof that the selected row is not
  shaped around only the select-plus-folded-constant fixture.

## Suggested Next

Start Step 3 - Prove Public Fallback And Nearby Same-Feature Cases. Add only
focused public-boundary tests for the selected fused-compare operand-producer
row where Route 7 evidence is absent, invalid, duplicate/conflicting,
mismatched, unfused, partially non-agreeing, or prepared-only, and prove the
prepared compare operand row remains unchanged. Keep expected strings,
baselines, wrappers, branch-control output, machine-printer output, suffix
mapping, fused-legality, hazard checks, target policy, and emitted-output
policy unchanged.

## Watchouts

- Keep the slice to one fused-compare operand-producer helper-oracle row
  family.
- Step 2 found no need for another agreement-reader implementation path; new
  code in Step 3 should be proof-only unless a public-boundary test exposes a
  real semantic gap.
- Preserve prepared fallback for absent route, invalid references,
  duplicate/conflict, mismatch, unfused, partial non-agreement, and
  prepared-only paths.
- Do not rewrite expected strings, baselines, wrapper output, branch-control
  output, machine-printer output, suffix mapping, fused legality, hazard
  checks, target policy, or emitted-output behavior.
- Reject testcase-shaped matching or fixture-name shortcuts.

## Proof

Ran delegated proof:
`cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log 2>&1`

Result: passed. Build reported no work to do; ctest passed
`backend_aarch64_branch_control_lowering`,
`backend_aarch64_instruction_dispatch`, and `backend_prepared_lookup_helper`.
Proof log: `test_after.log`.
