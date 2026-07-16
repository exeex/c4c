Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed Step 4 aggregate parameter alias slot metadata handoff.

`collect_aggregate_params()` now preserves an optional
`StructNameId`-bearing `function_.signature_param_type_refs[index]` on
`AggregateParamInfo`, and `materialize_aggregate_param_aliases(...)` passes
that ref into the existing layout-supplied
`declare_local_aggregate_slots(...)` overload. Metadata-bearing aggregate
parameters now attach structured type identity to their generated local
aggregate slots; legacy/no-id parameters stay on the existing text/layout
fallback.

## Suggested Next

Suggested Next: choose the next remaining Step 4 adapter deletion packet with a
structured ref already available at the slot declaration boundary, or move to
supervisor review if the intended aggregate slot declaration paths are now
covered.

## Watchouts

- This packet deliberately did not broaden into call returns, variadic
  `va_arg`, loads, allocas, or subobject/view construction paths.
- Aggregate parameter layout selection is unchanged: `StructNameId` refs use
  structured layout selection, while no-id/legacy parameters retain the
  existing rendered-text layout fallback.
- The supplied-layout `declare_local_aggregate_slots(...)` overload now records
  the optional parameter ref on `LocalAggregateSlots` without changing the
  previously selected layout.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains `100% tests passed, 0 tests failed
out of 6`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
reported before 6/6, after 6/6, result PASS.
