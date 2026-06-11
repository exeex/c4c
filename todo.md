Status: Active
Source Idea Path: ideas/open/206_route7_comparison_provenance_diagnostic_oracle.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The Route 7 Provenance Surface

# Current Packet

## Just Finished

Step 1 selected
`find_route7_materialized_condition_producer_identity(...)` in
`src/backend/mir/aarch64/codegen/comparison.cpp` as the first Route 7
comparison provenance reader boundary. The visible oracle row is the
materialized compare conditional branch path driven by
`lower_materialized_compare_condition_branch(...)` and covered by
`backend_aarch64_branch_control_lowering`.

The selected reader may consume only Route 7 materialized-condition comparison
provenance:

- comparison-producing `BinaryInst` pointer and instruction index
- condition value identity/name for the materialized `i1`
- lhs/rhs comparison operand values and producer/constant provenance exposed
  through the validated Route 7 comparison instruction record
- before-instruction boundary and current-block ownership needed to reject
  stale or invalid references

Prepared fallback must remain authoritative for:

- conditional-branch targets and target-pair records
- branch condition suffix, branch spelling, and final compare/branch emission
- fused legality and all target branch-policy decisions
- condition home / emitted-condition fallback when Route 7 is absent, invalid,
  duplicated, ambiguous, or mismatched
- hazards, emitted-register state, final instruction order, final records, and
  output-sensitive string/printer behavior outside this provenance reader

Existing targeted coverage already includes the valid Route 7 materialized
condition path and duplicate-reference fallback:

- `materialized_compare_branch_route7_provenance_matches_bir_identity()`
- `materialized_compare_branch_duplicate_route7_provenance_uses_emitted_fallback()`

Missing negative coverage for Step 2: absent Route 7 index/record,
invalid-reference status, condition value/name mismatch, producer instruction
index mismatch, binary pointer mismatch, lhs/rhs provenance mismatch, and
unchanged emitted-condition fallback strings for each rejection.

## Suggested Next

Execute Step 2: add a fail-closed Route 7 adapter around
`find_route7_materialized_condition_producer_identity(...)` that validates the
selected materialized-condition provenance against the prepared branch
condition before accepting it, then falls back to the existing prepared/BIR
materialized-condition path for every absent, invalid, duplicate, ambiguous, or
mismatched Route 7 fact.

## Watchouts

- Keep the first adapter scoped to the materialized-condition reader only; do
  not broaden into fused compare operand readers or branch-condition records in
  the same packet.
- The existing reader currently builds a Route 7 index from `context.bir_block`
  and falls back to `bir::find_materialized_condition_producer_identity(...)`
  only when Route 7 is unavailable; Step 2 should preserve that fallback and
  tighten acceptance rather than migrating branch policy into BIR schema.
- Add focused tests in
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp` near the
  two existing materialized compare Route 7 cases. Preserve the current
  expected strings for the fallback branch shape (`cbnz ...`, `b ...`) and the
  valid Route 7 compare/branch shape (`cmp ...`, `b.le ...`).
- Do not rewrite expected strings, downgrade supported tests, or treat helper
  renames as progress.

## Proof

Selection-only packet; no build or test required. Local proof:
`git diff --check -- todo.md`.

Recommended Step 2 proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_branch_control_lowering$' > test_after.log`.
