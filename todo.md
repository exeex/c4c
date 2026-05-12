Status: Active
Source Idea Path: ideas/open/173_aggregate_layout_identity_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Thread Structured Layout Identity To The Consumer

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by threading structured layout identity into the
selected local aggregate GEP consumer.

Files changed:
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `todo.md`

Behavior:
- `lookup_local_gep_layout_result()` now preserves legacy/no-structured-layout
  fallback when no structured table is supplied.
- When a structured table is supplied and contains the named `%struct...` key,
  local GEP lookup no longer accepts a legacy `TypeDeclMap` fallback if the
  keyed structured entry does not produce a structured layout.
- Existing structured-vs-legacy mismatch behavior remains authority-preserving:
  local GEP still succeeds through structured layout when the structured entry
  is valid and the legacy text disagrees.

Focused tests added:
- `local_gep_structured_mismatch` covers valid structured metadata with
  mismatched legacy type text and expects local GEP lowering to keep succeeding.
- `local_gep_rejects_structured_opaque_legacy_fallback` covers a keyed
  structured entry without usable structured layout and expects local GEP
  lowering to fail instead of silently using legacy layout.

## Suggested Next

Review or implement the next boundary called out by the supervisor, likely
starting from the same structured-layout status bits rather than broadening the
local GEP packet into globals, byval copies, or call ABI.

## Watchouts

- `declare_local_aggregate_slots()` still uses the shared selected-layout helper
  outside this packet; this slice only closes the selected local GEP lookup
  boundary.
- The rejection is keyed to a present structured table entry for the same named
  type; unrelated legacy-only named types retain the shared fallback behavior.
- `tests/backend/backend_prepare_structured_context_test.cpp` was inspected but
  did not need changes for this local GEP consumer packet.

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
