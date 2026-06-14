Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Narrow Value-Home Bridge

# Current Packet

## Just Finished

Step 3 implementation complete for the `names` value-home lookup candidate.

- Added `PreparedBirValueHomeAgreement` and
  `prepared_bir_value_home_agreement(...)` in `lookup_agreement.*`.
- Wired only `find_prepared_value_home_for_bir_value(...)` through the new
  agreement helper; direct prepared value-home lookup by `PreparedValueId` and
  `ValueNameId` remains on the existing index-or-scan compatibility path.
- The BIR-value agreement path now requires a named non-empty BIR value,
  prepared name-table resolution, exactly one backing value-home row for the
  prepared name, no duplicate backing value id, optional regalloc agreement
  when present, and supplied lookup indexes that point back to the same
  backing home.
- Added focused helper-test rows for an accepted named value-home lookup and
  fail-closed behavior for duplicate prepared names, duplicate prepared ids,
  stale value-id indexes, stale home indexes, missing supplied indexes,
  conflicting/duplicate regalloc ids, prepared/BIR spelling drift, immediate
  values, empty names, and null function locations.
- Preserved a public compatibility check showing duplicate prepared names still
  return the first indexed value-home row for direct prepared lookup, while the
  BIR-value agreement path rejects the same duplicate state.

## Suggested Next

Execute Step 4: add or confirm any remaining fail-closed proof rows for the
value-home lookup candidate.

Step 4 executor packet:

```text
to_subagent: c4c-executor
Objective: confirm the remaining fail-closed proof surface for idea 260 `names` value-home lookup candidate.
Plan Step: Step 4: Add Fail-Closed Proof Rows
Owned Files: tests/backend/bir/backend_prepared_lookup_helper_test.cpp, todo.md
Do Not Touch: plan.md, ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md, route-debug files, target-output baselines, unsupported expectations, same-block lookup code, semantic resolver code, control-flow code, store-source publication code, printer/debug code, value-home helper behavior unless an unproved Step 4 row exposes a real agreement bug.
Context: Step 3 already added the shared value-home agreement helper and covered the main positive and fail-closed rows. Step 4 should audit the plan's remaining fail-closed checklist against the focused helper test and add only missing proof rows, not broaden into another idea 260 candidate.
Task: Add any missing value-home helper proof rows for missing prepared ids/homes or null behavior not already covered, or record that Step 3's helper-test rows cover the Step 4 surface. Preserve public direct prepared lookup compatibility.
Proof: (cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
Done When: focused proof covers positive and fail-closed value-home rows, `todo.md` records whether Step 4 added rows or confirmed coverage, and no unrelated output or unsupported expectation files are changed.
If Blocked: stop and report the exact ambiguity; do not edit plan.md or source idea.
```

## Watchouts

- Keep this runbook limited to the selected `names` value-home lookup
  candidate.
- Do not treat this activation as approval for `PreparedBirModule` deletion,
  privatization, wrapping, broad aggregate retirement, or another idea 260
  candidate.
- Preserve prepared name tables, prepared value-home records, prepared lookup
  indexes, public aggregate compatibility, and current null fallback behavior.
- Do not rewrite route-debug, target output, baselines, unsupported
  expectations, helper/oracle status names, printer/debug strings,
  same-block lookup, semantic resolver API, control-flow, store-source
  publication, or backend lowering behavior to claim progress.
- The helper now distinguishes "public lookup returned a row" from "structured
  BIR agreement exists"; keep Step 4 proof-only unless it finds a real missing
  agreement check.
- Direct prepared lookup compatibility intentionally keeps first-emplace
  behavior for duplicates. Do not reinterpret that compatibility behavior as
  BIR agreement.
- `src/backend/prealloc/prepared_lookups.cpp` was owned for Step 3 but did not
  need changes because the selected public BIR-value entrypoint is inline in
  `value_locations.hpp`.

## Proof

Ran:

```bash
(cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
```

Result: passed. `test_after.log` shows `100% tests passed, 0 tests failed out
of 1`.
