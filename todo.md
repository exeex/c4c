Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed Step 4 aggregate load-result slot-state conversion.

`lower_memory_load_inst(...)` now threads the existing `LirLoadOp::type_str`
`LirTypeRef` into `declare_local_aggregate_slots(...)` for aggregate load result
slots created from local array, addressed pointer, and global aggregate load
paths. StructNameId-bearing aggregate load results now populate
`LocalAggregateSlots::type_ref` and use the structured layout lookup/fail-closed
route; non-load aggregate slot construction paths remain unchanged.

## Suggested Next

Suggested Next: migrate another remaining local aggregate slot construction path
that already has structured type metadata available, excluding call returns,
variadic aggregate `va_arg`, aggregate params, and unrelated subobject/view
construction unless the supervisor explicitly scopes them.

## Watchouts

- This packet deliberately did not broaden into call returns, variadic aggregate
  `va_arg`, aggregate params, or subobject/view construction paths.
- `LirLoadOp::type_str` may still be a no-id/text-only ref for legacy inputs;
  those aggregate load results continue through the documented no-id fallback in
  `declare_local_aggregate_slots(...)`.
- The aggregate layout probe in `lower_memory_load_inst(...)` still uses
  `lower_byval_aggregate_layout(load.type_str.str(), ...)` before slot
  declaration; this packet only migrated aggregate load result slot creation.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains `100% tests passed, 0 tests failed
out of 6`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
reported before 6/6, after 6/6, result PASS.
