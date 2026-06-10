# Current Packet

Status: Active
Source Idea Path: ideas/open/158_bir_comparison_condition_producer_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Populate or extract BIR facts from production comparison paths

## Just Finished

Step 4 added production-path prepared/BIR equivalence coverage for comparison
producer facts without switching consumers or changing AArch64 compare/branch
lowering.

`backend_prepared_lookup_helper_test.cpp` now builds a normal BIR comparison
branch, runs production prealloc phases that publish prepared branch-condition
and value-name facts, populates source-producer lookups through
`make_prepared_edge_publication_source_producer_lookups`, and verifies:

- prepared fused-compare operand producer facts match BIR
  `find_fused_compare_operand_producer_facts`;
- production-extracted folded integer constants are visible for both operands;
- prepared materialized-condition producer identity matches BIR
  `find_materialized_condition_producer_identity`;
- prepared helpers remain the oracle and no AArch64/prealloc consumers were
  switched.

## Suggested Next

Execute Step 5 from `plan.md`: switch one comparison or branch provenance
consumer to BIR producer queries, keeping prepared-equivalence coverage in place
and leaving legality, condition-code selection, branch emission, hazards,
emitted-register state, and final instruction policy on the existing
AArch64/prealloc authority.

## Watchouts

- `integer_constant` is a semantic producer fact, but AArch64 immediate
  encodability remains target policy.
- `can_fuse_with_branch` is prepared/AArch64 legality, not BIR provenance.
- Keep fused-compare legality, condition-code selection, branch emission
  strategy, final instruction records/errors, hazard handling, emitted-register
  state, and AArch64 compare/branch instruction selection outside BIR.
- Consumer switches must be one provenance read at a time; retain prepared
  oracle checks while switching.
- Do not claim progress through expectation rewrites, testcase-shaped
  shortcuts, or single-case proof that omits nearby constant and
  materialized-condition coverage.

## Proof

Focused proof passed before the full backend subset:
`ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`.

Delegated full backend proof passed:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`.

`test_after.log` reports 179 backend tests passed, 0 failed.
