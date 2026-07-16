Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed Step 4 call-return sret aggregate slot metadata handoff.

`lower_call_inst()` now tracks the selected sret storage type with both the
rendered storage text and an optional `StructNameId`-bearing `LirTypeRef`.
Direct sret returns carry `call.return_type` into
`declare_local_aggregate_slots(...)` when it has structured identity, and the
store-derived sret correction switches both the storage text and optional ref
to `LirStoreOp::type_str`. Legacy/no-id return paths still pass no structured
ref and keep the existing rendered-text fallback.

## Suggested Next

Suggested Next: supervisor review of the covered Step 4 aggregate slot
declaration paths, or one final targeted packet if another expired adapter
boundary remains in scope.

## Watchouts

- This packet deliberately did not broaden into aggregate parameters,
  variadic `va_arg`, loads, allocas, PHI, or subobject/view paths.
- Store-derived sret correction still detects aggregate sret from the existing
  return-info path; the new structured ref is consumed at the local aggregate
  slot declaration boundary, where metadata-bearing refs fail closed and
  no-id refs retain fallback behavior.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains `100% tests passed, 0 tests failed
out of 6`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
reported before 6/6, after 6/6, result PASS.
