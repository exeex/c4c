# Current Packet

Status: Active
Source Idea Path: ideas/open/158_bir_comparison_condition_producer_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Switch comparison and branch provenance consumers

## Just Finished

Step 5 switched the cast-producer selection at the start of
`lower_fused_compare_branch_from_emitted_cast` from
`find_prepared_fused_compare_operand_producer` to
`bir::find_comparison_operand_producer`.

`comparison.cpp` now selects the LHS/RHS cast operand only when the BIR
comparison producer identity reports `bir::ComparisonProducerKind::Cast`,
obtains the `bir::CastInst` from the BIR producer instruction, and preserves
the BIR producer instruction index for the existing later cast-operand load
lookup. The later RHS integer-constant read and cast-operand load-producer read
remain on the prepared helper in this packet. Prepared branch facts, cast
opcode/type checks, load address lookup, compare emission, condition-code
selection, branch emission, hazards, emitted-register state, and final
instruction policy remain on their existing paths.

## Suggested Next

Continue Step 5 with exactly one additional supervisor-selected provenance
consumer switch, such as the later RHS integer-constant read or the
cast-operand load-producer read in `lower_fused_compare_branch_from_emitted_cast`,
or move to Step 6 acceptance validation if no further consumer switch is
desired for this idea.

## Watchouts

- `lower_fused_compare_branch_from_emitted_cast` now reads only the initial
  cast-producer selection from BIR; its later RHS integer-constant and
  cast-operand load-producer reads still belong to separate provenance
  consumers.
- Do not combine the next provenance read with register target policy, branch
  legality, cast opcode/type checks, condition-code, emission, final-record,
  hazard, or emitted-register changes.
- Keep prepared-oracle equivalence checks green while switching consumers one
  at a time.
- Do not claim progress through expectation rewrites, testcase-shaped
  shortcuts, or single-case proof that omits nearby constant and
  materialized-condition coverage.

## Proof

Delegated proof passed:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`.

`test_after.log` reports the build and backend subset passed, including 179
backend tests with 0 failures.
