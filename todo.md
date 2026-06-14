Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Narrow Semantic Resolver API

# Current Packet

## Just Finished

Completed Step 3: implemented the narrow semantic resolver agreement API for
the `names` semantic resolver candidate.

Implemented:

- Added `PreparedSemanticNameAgreementStatus` with explicit `Unavailable`,
  `Available`, and `Conflicted` states.
- Added typed result structs and helpers for prepared function names, block
  labels, and value names in `src/backend/prealloc/lookup_agreement.hpp` and
  `src/backend/prealloc/lookup_agreement.cpp`.
- Kept helpers const and non-interning: they use only existing prepared
  `find(...)` and `spelling(...)` facts, plus `bir::NameTables::block_labels`
  when a raw BIR block-label id participates in agreement.
- Left `src/backend/prealloc/control_flow.hpp` unchanged so
  `resolve_prepared_*` wrappers and direct `names.*.find(...)` remain
  compatibility surfaces.
- Added focused helper-test coverage for positive function/block/value rows,
  absent rows, invalid raw block-label ids, raw-id spelling drift, copied or
  corrupted prepared-name table round-trip mismatch, direct-find compatibility,
  and non-interning table-size preservation.

## Suggested Next

Execute Step 4: add or audit remaining fail-closed proof rows for the semantic
resolver helpers.

Suggested Step 4 packet:

- Owned files: `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` and
  `todo.md`.
- Audit whether the new helper-test rows are enough for empty spellings,
  missing prepared ids, invalid raw BIR ids, raw-id spelling drift,
  prepared/BIR spelling drift, copied or corrupted prepared-name table
  mismatch, and accidental interning attempts.
- Add missing proof rows only; do not change helper behavior unless a proof gap
  exposes a real semantic agreement bug.
- Preserve route-debug, target-output, same-block lookup, value-home lookup,
  control-flow, store-source, printer/debug, and backend lowering behavior.
- Proof command:
  `(cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1`

## Watchouts

- Keep this runbook limited to the selected `names` semantic resolver API
  candidate.
- Do not treat this activation as approval for `PreparedBirModule` deletion,
  privatization, wrapping, broad aggregate retirement, or another idea 260
  candidate.
- Preserve prepared name tables, public aggregate compatibility, existing
  construction-time interning behavior, and current absent/null resolver
  behavior.
- Do not rewrite route-debug, target output, baselines, unsupported
  expectations, helper/oracle status names, printer/debug strings,
  same-block lookup, value-home lookup, control-flow, store-source
  publication, or backend lowering behavior to claim progress.
- Do not use `prepared_named_value_id(...)` as proof of resolver safety; it
  intentionally mutates prepared value names during construction.
- Treat raw BIR ids and raw spelling fallback as compatibility inputs only.
  They become semantic resolver agreement only after the new helper validates
  spelling, id, and prepared-table round trip.
- BIR function and value rows currently expose spellings directly; raw-id
  validation is concrete for block labels through `bir::NameTables`. Do not
  invent new BIR ids for function or value names in this runbook.
- The agreement helper should fail closed, not downgrade tests or rewrite
  backend output expectations to claim progress.
- The Step 3 implementation left `resolve_prepared_*` wrappers unchanged.
  Step 4 should treat those wrappers as compatibility adapters unless the
  supervisor explicitly delegates behavior-preserving wiring.

## Proof

Ran
`(cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1`;
passed. `test_after.log` shows `100% tests passed, 0 tests failed out of 1`.
