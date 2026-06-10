# Current Packet

Status: Active
Source Idea Path: ideas/open/158_bir_comparison_condition_producer_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add the BIR comparison producer relationship surface

## Just Finished

Step 2 added a BIR-owned comparison/condition producer query surface without
changing AArch64 compare/branch lowering.

New BIR query surface:

- `bir::find_comparison_operand_producer` exposes immediate operands,
  same-block load-local/load-global/cast/binary/select producers,
  producer instruction indexes, produced values, and folded integer constants.
- `bir::find_fused_compare_operand_producer_facts` returns optional lhs/rhs
  semantic producer facts for a fused compare operand pair.
- `bir::find_materialized_condition_producer_identity` exposes a
  comparison-producing `BinaryInst`, condition value name, producer instruction
  index, and lhs/rhs semantic producer facts for named condition values.

The implementation fails closed for missing producers, producers after the
query index, duplicate same-block producers, non-named condition values, and
non-comparison binary condition producers. It does not carry
`can_fuse_with_branch`, target labels, condition suffixes, immediate
encodability, scratch choices, hazards, emitted-register state, diagnostics, or
final instruction records.

`backend_prepared_lookup_helper_test.cpp` now covers BIR fixture queries for
immediate constants, folded same-block constants, load/cast/binary/select
producers, materialized comparison binary identity, missing and after-index
producers, duplicate producers, and non-comparison condition producers.

## Suggested Next

Execute Step 3 from `plan.md`: add prepared/BIR equivalence checks for fused
compare operand producer answers and materialized-condition producer answers.
Keep prepared helpers as the oracle and do not switch consumers.

## Watchouts

- `integer_constant` is a semantic producer fact, but AArch64 immediate
  encodability remains target policy.
- The Step 2 BIR query is fixture-backed only; production-lowered comparison
  coverage belongs to Step 4.
- `can_fuse_with_branch` is prepared/AArch64 legality, not BIR provenance.
- Keep fused-compare legality, condition-code selection, branch emission
  strategy, final instruction records/errors, hazard handling, emitted-register
  state, and AArch64 compare/branch instruction selection outside BIR.
- Do not switch comparison or branch consumers before prepared-oracle
  equivalence is proven for the specific provenance read.
- Do not claim progress through expectation rewrites, testcase-shaped
  shortcuts, or single-case proof that omits nearby constant and
  materialized-condition coverage.

## Proof

Focused proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`.

Full backend proof passed:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`.

`test_after.log` reports 179 backend tests passed, 0 failed.
