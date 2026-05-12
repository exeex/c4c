Status: Active
Source Idea Path: ideas/open/173_aggregate_layout_identity_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fail Closed On Metadata-Rich Mismatches

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by making the selected local aggregate GEP consumer
fail closed on metadata-rich structured-vs-legacy layout mismatches.

Files changed:
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `todo.md`

Behavior:
- `lookup_local_gep_layout_result()` now rejects a lookup result with
  `structured_text_mismatch`, so a valid structured entry whose legacy
  `TypeDeclMap` layout disagrees no longer passes local GEP lowering.
- The committed Step 2 keyed-entry rejection remains: when a structured table
  contains the named `%struct...` key but cannot produce a structured layout,
  local GEP lookup fails instead of silently using the legacy layout.
- The legacy/no-metadata fallback remains only when no structured table is
  supplied, or when the structured table has no entry for that named type and
  the route is still an explicit compatibility path.

Focused tests updated:
- `local_gep_rejects_structured_mismatch` covers valid structured metadata with
  mismatched legacy type text and now expects local GEP lowering to fail closed.
- `local_gep_rejects_structured_opaque_legacy_fallback` covers a keyed
  structured entry without usable structured layout and expects local GEP
  lowering to fail instead of silently using legacy layout.

## Suggested Next

Proceed to `plan.md` Step 4 for focused layout behavior and collision coverage
at the selected local aggregate GEP boundary.

## Watchouts

- Step 3 intentionally stayed inside local aggregate GEP lookup; globals, byval
  copies, call ABI, and broad backend layout migration remain out of scope.
- The no-metadata compatibility path still exists for legacy hand-authored
  inputs, but metadata-rich mismatches at this selected consumer now fail
  closed.
- `backend_prepare_structured_context_test` already covers the
  `structured_text_mismatch` lookup bit; this packet changed the local GEP
  consumer response to that bit.

## Proof

Ran exactly:

```sh
{ cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test && ctest --test-dir build -R 'backend_prepare_structured_context|backend_lir_to_bir_notes' --output-on-failure; } > test_after.log 2>&1
```

Result: passed. `test_after.log` is the proof log.

Regression guard:
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: passed; focused CTest count stayed `2/2` because the new coverage is
  inside an existing backend test binary.

Additional backend validation:
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Result: passed; `109` backend tests run, `0` failed, with the existing
  disabled MIR trace tests not run.
