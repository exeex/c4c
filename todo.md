Status: Active
Source Idea Path: ideas/open/608_prepared_global_data_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish direct global-symbol base-plus-offset authority

# Current Packet

## Just Finished

- Completed Step 3 (`Publish direct global-symbol base-plus-offset authority`)
  from `plan.md`.
- Verified the current prepared/prealloc publication path already publishes
  direct global-symbol base-plus-offset authority from proven inputs without
  RV64 target lowering or expectation changes. Representative prepared dumps
  show direct global-symbol rows gaining concrete layout authority while
  retaining fail-closed identity/range checks.
- Focused movement: `src/pr79737-2.c`, `src/pr82387.c`, `src/pr68624.c`, and
  `src/pr57568.c` compile to RV64 objects. `src/990326-1.c` moves past the
  direct global-symbol base-plus-offset stop and reaches downstream prepared
  move-bundle consumer authority.
- Classified residual `src/pr36034-1.c` and `src/pr91137.c` as not Step 3
  direct-base-offset blockers: their prepared rows already carry direct
  global-symbol base-plus-offset facts, but remaining unsupported rows still
  have `layout_authority=unknown` for broader selected/aggregate object-data
  authority.

## Suggested Next

Delegate Step 4 (`Publish selected object-data authority`) to publish or
classify selected/aggregate global object-data layout authority for remaining
prepared global-memory rows such as `src/pr36034-1.c` and `src/pr91137.c`.
Suggested proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

## Watchouts

- Do not solve the remaining `pr36034-1.c`/`pr91137.c` rows by weakening
  diagnostics or adding named-case matching; the evidence points at aggregate
  object-data authority, not direct base-plus-offset address publication.
- Keep RV64 global symbol emission/lowering, relocation-record emission,
  unsupported markers, allowlists, timeout/accounting files, and test
  expectations out of the next packet unless the supervisor explicitly changes
  route.
- `src/990326-1.c` is already beyond this family and should stay with the
  prepared move-bundle owner.

## Proof

- Ran the delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

- Result: passed, 346 backend tests, 0 failures.
- Proof log: `test_after.log`.
- Focused representative sanity checks:
  `src/pr79737-2.c`, `src/pr82387.c`, `src/pr68624.c`, and `src/pr57568.c`
  compile to RV64 objects; `src/990326-1.c` progresses to
  `unsupported_move_bundle_target_shape`; `src/pr36034-1.c` and
  `src/pr91137.c` remain at the broader prepared global-memory fact diagnostic
  with direct global-symbol base-plus-offset facts present but aggregate
  object-data layout authority still missing.
