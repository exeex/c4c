# Current Packet

Status: Active
Source Idea Path: ideas/open/158_bir_comparison_condition_producer_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Switch comparison and branch provenance consumers

## Just Finished

Step 5 switched the selected-operand provenance check used by
`fused_compare_uses_selected_operand` from the prepared fused-compare operand
producer helper to BIR fused-compare operand producer facts.

`comparison.cpp` now asks
`bir::find_fused_compare_operand_producer_facts` for the branch lhs/rhs pair
and treats `bir::ComparisonProducerKind::Select` as the selected-operand
provenance signal. The surrounding prepared branch facts, fused-compare
legality, condition-code selection, branch emission, missing-publication
lowering, hazards, emitted-register state, and final instruction policy remain
on their existing AArch64/prealloc paths.

Prepared/BIR equivalence coverage remains in place, and the existing selected
global-load fused-branch tests still cover both lhs and rhs selected operand
behavior.

## Suggested Next

Continue Step 5 with exactly one additional supervisor-selected provenance
consumer switch, or move to Step 6 acceptance validation if no further consumer
switch is desired for this idea.

## Watchouts

- The selected-operand gate now reads BIR semantic producer provenance, but the
  downstream publication and branch lowering helpers still intentionally use
  prepared/AArch64 authority.
- Do not combine the next provenance read with branch legality, condition-code,
  emission, final-record, hazard, or emitted-register changes.
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
