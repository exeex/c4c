# Current Packet

Status: Active
Source Idea Path: ideas/open/158_bir_comparison_condition_producer_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Switch comparison and branch provenance consumers

## Just Finished

Step 5 switched the LHS allowed-load producer check in
`lower_stack_home_fused_compare_branch` from
`find_prepared_fused_compare_operand_producer` to
`bir::find_comparison_operand_producer`.

`comparison.cpp` now permits named LHS stack-home fused compare operands only
when the BIR comparison producer identity is absent or reports
`bir::ComparisonProducerKind::LoadLocal` / `LoadGlobal`, preserving the
existing fail-closed rejection for other present producer kinds. Immediate
encodability, stack home lookup, frame slot emission, scratch register choice,
branch emission, condition-code selection, hazards, emitted-register state, and
final instruction policy remain on their existing paths.

## Suggested Next

Continue Step 5 with exactly one additional supervisor-selected provenance
consumer switch if any remain, or move to Step 6 acceptance validation if no
further consumer switch is desired for this idea.

## Watchouts

- `lower_stack_home_fused_compare_branch` now reads the LHS allowed-load
  producer identity from BIR, but the remaining prepared helper call sites in
  `comparison.cpp` still belong to separate provenance consumers.
- Do not combine the next provenance read with register target policy, branch
  legality, condition-code, emission, final-record, hazard, or emitted-register
  changes.
- Keep prepared-oracle equivalence checks green while switching consumers one
  at a time.
- Do not claim progress through expectation rewrites, testcase-shaped
  shortcuts, or single-case proof that omits nearby constant and
  materialized-condition coverage.

## Proof

Delegated proof passed:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`.

`test_after.log` reports the backend subset passed, including 179 backend tests
with 0 failures.
