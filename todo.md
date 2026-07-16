Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed Step 4 aggregate alloca slot-state conversion.

`lower_memory_alloca_inst(...)` now threads the existing `LirAllocaOp::type_str`
`LirTypeRef` into `declare_local_aggregate_slots(...)` for the aggregate alloca
construction path. StructNameId-bearing aggregate allocas now populate
`LocalAggregateSlots::type_ref` and use the structured layout lookup/fail-closed
route; scalar, local array, vector, and unrelated text-only callers remain on
their existing paths.

## Suggested Next

Suggested Next: choose one remaining non-alloca local aggregate slot
construction path that already has structured type metadata available, thread
that `LirTypeRef` into the slot state, and leave no-id/text-only callers on the
explicit legacy fallback.

## Watchouts

- This packet deliberately did not broaden into load/store, call return, or
  variadic aggregate construction paths.
- `LirAllocaOp::type_str` may still be a no-id/text-only ref for legacy inputs;
  those allocas continue through the documented no-id fallback in
  `declare_local_aggregate_slots(...)`.
- StructNameId-bearing aggregate alloca slots now rely on the structured lookup
  before any later leaf-slot collection, so unresolved structured metadata fails
  closed.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains `100% tests passed, 0 tests failed
out of 6`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
reported before 6/6, after 6/6, result PASS.
