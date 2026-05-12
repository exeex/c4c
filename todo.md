Status: Active
Source Idea Path: ideas/open/173_aggregate_layout_identity_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Layout Behavior And Collision Coverage

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by confirming the selected local aggregate GEP
boundary already has focused layout behavior and collision coverage.

Coverage satisfied:
- `local_aggregate_raw_i8_gep_byte_slice` protects raw byte-slice layout
  behavior by requiring an `i8` GEP at byte offset 8 within `%struct.s9 = type
  { [9 x i8] }` to lower without a local-memory GEP failure note.
- `local_aggregate_raw_float_leaf_byte_slice` protects scalar leaf layout
  behavior by requiring an `i8` GEP at byte offset 8 within `%struct.hfa13 =
  type { float, float, float }` to resolve to a valid local aggregate leaf.
- `local_aggregate_raw_float_tail_memcpy` protects tail layout behavior by
  requiring the byte offset 8 projection into `%struct.hfa14 = type { float,
  float, float, float }` to remain usable as the destination of an 8-byte
  local aggregate memcpy.
- `local_gep_rejects_structured_mismatch` covers the metadata-rich
  structured-vs-legacy mismatch case: the structured `%struct.Pair` declaration
  is `{ i32, i32 }`, the legacy `type_decls` spelling is `{ i64, i64 }`, and
  local GEP lowering now fails closed instead of accepting the equal rendered
  struct name as identity.
- `local_gep_rejects_structured_opaque_legacy_fallback` covers the keyed
  structured-entry collision case: `%struct.Pair` is present in the structured
  table but opaque/unusable, while legacy `type_decls` has `{ i64, i64 }`, and
  local GEP lowering fails instead of falling back through the spelling-keyed
  legacy layout.

No missing Step 4 test gap was found for the selected local aggregate GEP
boundary, so no test or implementation edits were made.

## Suggested Next

Proceed to `plan.md` Step 5 to validate and summarize the selected local
aggregate GEP boundary without broadening into globals, byval copies, call ABI,
or unrelated backend routes.

## Watchouts

- Step 4 is evidence-only: the layout behavior and collision cases are already
  covered by committed focused tests in `backend_lir_to_bir_notes_test`.
- The selected boundary remains local aggregate GEP lookup only; globals, byval
  copies, call ABI, and broad backend route migration remain out of scope.
- The no-metadata compatibility path still exists for legacy hand-authored
  inputs, but metadata-rich mismatches and keyed structured-entry collisions at
  this selected consumer fail closed.
- `backend_prepare_structured_context_test` covers the
  `structured_text_mismatch` lookup bit, while `backend_lir_to_bir_notes_test`
  covers the local GEP consumer response.

## Proof

No proof command was rerun for Step 4 because this packet made no test or code
edits.

Committed Step 3 proof remains the focused validation for the cited coverage:

```sh
{ cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test && ctest --test-dir build -R 'backend_prepare_structured_context|backend_lir_to_bir_notes' --output-on-failure; } > test_after.log 2>&1
```

Result from committed Step 3: passed. `test_after.log` is the existing proof
log.

Regression guard:
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: passed; focused CTest count stayed `2/2` because the new coverage is
  inside an existing backend test binary.

Additional backend validation:
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Result: passed; `109` backend tests run, `0` failed, with the existing
  disabled MIR trace tests not run.
