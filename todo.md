# Current Packet

Status: Active
Source Idea Path: ideas/open/158_bir_comparison_condition_producer_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Bridge prepared answers to BIR comparison queries

## Just Finished

Step 3 added prepared/BIR equivalence coverage for comparison producer facts
without switching consumers or changing AArch64 compare/branch lowering.

Equivalence coverage added in `backend_prepared_lookup_helper_test.cpp`:

- prepared `find_prepared_fused_compare_operand_producer_facts` now compares
  against BIR `find_fused_compare_operand_producer_facts` for select and
  folded-constant operands;
- immediate fused-compare operands compare as semantic integer constants;
- rhs-only availability matches when the lhs producer is unavailable;
- non-fused/materialized-bool and fully unavailable paths fail closed;
- prepared `find_prepared_materialized_condition_producer` now compares
  against BIR `find_materialized_condition_producer_identity` for binary
  condition producers, condition value name identity, instruction index, and
  lhs/rhs producer availability.

Prepared helpers remain the oracle. BIR equivalence is still fixture-backed;
normal production comparison/branch extraction is not proven until Step 4.

## Suggested Next

Execute Step 4 from `plan.md`: populate or extract BIR comparison producer
facts from normal production comparison/branch paths and prove them against the
prepared semantic oracle. Do not switch AArch64 consumers yet.

## Watchouts

- `integer_constant` is a semantic producer fact, but AArch64 immediate
  encodability remains target policy.
- The Step 3 BIR query proof is fixture-backed only; production-lowered comparison
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

Focused proof passed after Step 3:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`.

Full backend proof passed:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`.

`test_after.log` reports 179 backend tests passed, 0 failed.
