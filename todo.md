Status: Active
Source Idea Path: ideas/open/608_prepared_global_data_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Recheck selected object-data authority residuals

# Current Packet

## Just Finished

- Completed Step 4 (`Recheck selected object-data authority residuals`) from
  `plan.md`.
- No owned code edits were needed: the current selected object-data publisher
  and verifier already classify coherent, missing, conflicting, unsupported,
  and pre-prepared semantic states fail-closed without RV64 target or
  expectation changes.
- Rechecked the Step 3 watchouts. `src/pr36034-1.c` and `src/pr91137.c` now
  stop at `RV64 object route requires supported prepared global memory facts`,
  so they are not selected/aggregate object-data authority residuals for Step 4.
- Rechecked remaining `prepared selected object-data contract` rows in the
  current RV64 backend logs. They split into `unsupported_but_coherent`
  unsupported-marker rows or a `missing_object_label` producer-label row, not a
  missing selected/aggregate object-data layout fact that can be repaired in the
  Step 4 prepared/prealloc publication files.

## Suggested Next

Delegate Step 5 (`Prove prepared/global handoff and close readiness`) to
summarize the prepared/global movement and residual owner split for lifecycle
review. Suggested proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

## Watchouts

- Do not route `src/pr36034-1.c` or `src/pr91137.c` back through selected
  object-data authority; their current first diagnostic is prepared global
  memory facts, and Step 4 has no selected/aggregate producer fact to publish
  for them.
- The remaining selected object-data rows are still intentionally fail-closed:
  `unsupported_but_coherent` rows carry unsupported-marker authority, while
  `src/20050929-1.c` currently lacks selected object-label authority before the
  prepared object-data publisher can prove a concrete object-data row.
- Keep RV64 global symbol emission/lowering, relocation-record emission,
  unsupported markers, allowlists, timeout/accounting files, and test
  expectations out of this route unless the supervisor explicitly changes
  ownership.

## Proof

- Ran the delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

- Result: passed, 346 backend tests, 0 failures.
- Proof log: `test_after.log`.
- Focused representative sanity checks:
  `src/pr36034-1.c` and `src/pr91137.c` now report prepared global-memory fact
  requirements rather than selected object-data authority. Current selected
  object-data residuals remain fail-closed as `unsupported_but_coherent` or
  `missing_object_label` rather than ambiguous selected/aggregate layout
  authority.
