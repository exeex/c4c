Status: Active
Source Idea Path: ideas/open/117_aarch64_comparison_fused_compare_publication_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Boundary Audit And Closure Package

# Current Packet

## Just Finished

Step 5 - Final Boundary Audit And Closure Package completed the final boundary
audit for idea 117 without implementation changes.

Migrated facts:

- Step 2 exposed prepared dump/test visibility for block-entry publication
  sections and register facts without weakening supported expectations.
- Step 3 moved fused-compare operand producer ownership behind shared
  `prepare::find_prepared_fused_compare_operand_producer_facts` /
  `PreparedFusedCompareOperandProducerFacts`; `comparison.cpp` now adapts that
  shared fact to AArch64 operand emission.
- Step 4 exposed the current-block publication destination register through
  shared prepared facts and moved local collection/register lookup out of
  `comparison.cpp` into the reusable AArch64 dispatch-publication adapter.
- Materialized compare join targets are consumed through shared prepared
  authority: authoritative branch-owned join transfer lookup, materialized
  compare join context, and published compare-join continuation targets.

Keep-local decisions:

- `comparison.cpp` retains AArch64 condition suffix spelling, compare opcode
  and immediate encodability checks, branch assembly text, scratch/register
  selection, operand view coercion, and target fallback emission.
- The dispatch-publication adapter owns AArch64 register-name parsing and
  register-bank filtering for prepared current-block publication facts; the
  shared prepared contract remains target-neutral.
- The remaining raw terminator checks in conditional branch lowering are
  validation against prepared branch metadata, not source-route recovery.

## Suggested Next

Closure-ready for supervisor review. No executor blocker remains for idea 117.

## Watchouts

- AST-backed audit of `comparison.cpp` confirmed the migrated helper families
  call shared `prepare` lookup/query APIs or the dispatch-publication adapter.
- No same-block rescans, BIR-name routing, raw-terminator recovery, named-case
  shortcuts, expectation downgrades, or unsupported-path rewrites were used for
  this closure package.
- Proof coverage includes AArch64 instruction dispatch, authoritative join
  ownership, block-entry publication facts, prepared lookup helper coverage,
  and prepared BIR dump contract sections.

## Proof

Passed. Exact delegated proof command:

`(cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prepare_authoritative_join_ownership|backend_prealloc_block_entry_publications|backend_prepared_lookup_helper|backend_cli_dump_prepared_bir_exposes_contract_sections)$' --output-on-failure) > test_after.log 2>&1`

Result: build succeeded, `100% tests passed, 0 tests failed out of 5`; proof
log is `test_after.log`.

Supervisor acceptance validation:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: monotonic focused regression guard passed with no pass/fail delta, and
the broader backend subset passed `179/179`.

Closure status: closure-ready; no exact blockers found.
