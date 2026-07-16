Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed Step 4 variadic aggregate `va_arg` slot-state conversion.

`lower_runtime_intrinsic_inst(...)` now threads the existing
`LirVaArgOp::type_str` `LirTypeRef` into `declare_local_aggregate_slots(...)`
for aggregate `va_arg` result slots. Metadata-bearing aggregate `va_arg` result
refs now use structured layout lookup/fail-closed behavior; absent/no-id refs
remain on the documented rendered-text fallback path.

## Suggested Next

Suggested Next: migrate another remaining local aggregate slot construction path
that already has structured type metadata available, excluding variadic
aggregate `va_arg` unless the supervisor explicitly scopes follow-up validation
or cleanup for that path.

## Watchouts

- This packet deliberately did not broaden into call returns, aggregate params,
  loads, allocas, or subobject/view construction paths.
- `LirVaArgOp::type_str` may still be a no-id/text-only ref for legacy inputs;
  those aggregate `va_arg` results continue through the documented no-id
  fallback in `declare_local_aggregate_slots(...)`.
- The aggregate layout probe in the `va_arg` aggregate path still uses
  `lower_byval_aggregate_layout(va_arg.type_str.str(), ...)` before slot
  declaration; this packet only migrated aggregate `va_arg` result slot
  creation.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains `100% tests passed, 0 tests failed
out of 6`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
reported before 6/6, after 6/6, result PASS.
