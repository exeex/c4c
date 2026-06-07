Status: Active
Source Idea Path: ideas/open/117_aarch64_comparison_fused_compare_publication_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Expose Fused-Compare Operand Producer Facts

# Current Packet

## Just Finished

Step 3 - Expose Fused-Compare Operand Producer Facts has a shared prepared
operand producer contract consumed by AArch64 comparison lowering.

Migrated facts:

- `prepare::PreparedFusedCompareOperandProducerFacts` now carries optional
  lhs/rhs `PreparedFusedCompareOperandProducer` records.
- `prepare::find_prepared_fused_compare_operand_producer_facts` exposes the
  branch-condition-level prepared query by composing the existing single-operand
  producer lookup and failing closed when neither side has facts.
- `comparison.cpp` now consumes that shared prepared contract for branch record
  construction instead of locally packaging lhs/rhs producer facts around
  `find_prepared_fused_compare_operand_producer`.
- `backend_prepared_lookup_helper` now proves the shared query exposes lhs/rhs
  fused-compare facts and rejects non-fused branch conditions.

Why this is not target-local:

- The new lhs/rhs producer grouping lives in shared prepared lookup code and is
  keyed by `PreparedBranchCondition`, block label, BIR block, and producer
  lookup tables before any AArch64 scalar view, condition suffix, compare opcode,
  operand spelling, branch emission, or fallback policy is selected.

## Suggested Next

Delegate Step 4 to replace `comparison.cpp`'s current-block publication
iteration/duplicate register helper with a shared prepared publication query or
small shared AArch64 dispatch-publication adapter that consumes the already
visible `PreparedBlockEntryPublication` fact.

## Watchouts

- `comparison.cpp` still has a context adapter for fallback construction of
  `PreparedEdgePublicationSourceProducerLookups` when prebuilt function lookups
  are not present; the lhs/rhs fact ownership itself is now shared prepared
  code.
- Step 4 should keep AArch64 register parsing and operand/register view
  selection target-local while moving current-block publication fact selection
  out of local iteration.

## Proof

Passed. Exact delegated proof command:

`(cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prepare_authoritative_join_ownership|backend_prealloc_block_entry_publications|backend_prepared_lookup_helper|backend_cli_dump_prepared_bir_exposes_contract_sections)$' --output-on-failure) > test_after.log 2>&1`

Result: `100% tests passed, 0 tests failed out of 5`; proof log is
`test_after.log`.

Supervisor acceptance validation:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: monotonic focused regression guard passed with no pass/fail delta, and
the broader backend subset passed `179/179`.
