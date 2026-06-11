Status: Active
Source Idea Path: ideas/open/182_phase_e_route4_publication_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate And Hand Off

# Current Packet

## Just Finished

Step 5 from `plan.md` is complete: finalized validation and supervisor
handoff notes for the selected Route 4 consumer migration without claiming
broader prepared API contraction.

Completed slice changed:
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp`,
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`,
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, and `todo.md`.

Selected reader:
`current_block_entry_publication_register(...)` now has a Route 4
publication-identity boundary with prepared fallback/oracle behavior preserved.
Tests cover the route-present match, unavailable Route 4 fallback, invalid or
mismatched Route 4 fallback, missing prepared-index fail-closed behavior, and
Route 4 facade validation/status equivalence against the direct validator.

No prepared publication helper was deleted, privatized, or narrowed. This slice
does not claim broad prepared API removal, broad consumer migration, target
policy movement into BIR, or publication construction ownership changes.

## Suggested Next

Ask the plan-owner for the lifecycle closure decision for
`ideas/open/182_phase_e_route4_publication_view_consumer_migration.md`.

## Watchouts

- Residual risk is limited to the selected reader boundary:
  `current_block_entry_publication_register(...)`.
- Prepared data remains the register spelling and destination-home authority;
  Route 4 supplies preferred semantic identity only when available and valid.
- Keep prepared publication helpers public until later Phase E follow-up ideas
  migrate the remaining publication consumers.
- Do not treat the rejected full-suite candidate as a new accepted baseline;
  it regressed three c_testsuite AArch64 backend tests relative to
  `test_baseline.log`.

## Proof

No new build/test was required for this Step 5 docs/todo-only handoff packet.
The completed selected-consumer slice already has focused proof preserved in
`test_after.log`:

```bash
cmake --build build --target backend_aarch64_instruction_dispatch_test backend_prealloc_block_entry_publications_test backend_prepared_lookup_helper_test &&
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prealloc_block_entry_publications|backend_prepared_lookup_helper)$' --output-on-failure
```

Result: 3/3 tests passed.

Regression guard passed against `test_before.log` with
`--allow-non-decreasing-passed` for the same focused 3-test subset.

The hook-produced full-suite `test_baseline.new.log` was rejected and must not
replace `test_baseline.log`: `test_baseline.log` records 3427/3427 passing,
while `test_baseline.new.log` records 3425/3428 passing with three regressed
c_testsuite AArch64 backend tests:
`c_testsuite_aarch64_backend_src_00119_c`,
`c_testsuite_aarch64_backend_src_00123_c`, and
`c_testsuite_aarch64_backend_src_00195_c`.
