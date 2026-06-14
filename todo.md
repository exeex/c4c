Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Fail-Closed Proof Rows

# Current Packet

## Just Finished

Step 4 proof audit complete for the `names` value-home lookup candidate.

- Rechecked the Step 4 fail-closed checklist against
  `verify_prepared_bir_value_home_agreement_boundary()`.
- Step 3 already covered the accepted row, duplicate prepared names, duplicate
  prepared ids, stale value-id indexes, stale home pointers, missing supplied
  value-id indexes, conflicting/duplicate regalloc ids, prepared/BIR spelling
  drift, immediate values, empty names, null function locations, absent
  optional indexes, and public direct-lookup duplicate compatibility.
- Added the remaining explicit rows for an uninterned BIR value name missing
  from prepared `ValueNameId` tables, a supplied home index missing the agreed
  `PreparedValueId`, and a prepared function-location record with the agreed
  `ValueNameId` but no backing value-home row.
- Preserved public direct prepared lookup compatibility and made no helper
  behavior changes.

## Suggested Next

Execute Step 5: broader validation and closure readiness for the value-home
lookup candidate.

Step 5 executor packet:

```text
to_subagent: c4c-executor
Objective: validate closure readiness for idea 260 `names` value-home lookup candidate.
Plan Step: Step 5: Broader Validation and Closure Readiness
Owned Files: todo.md
Do Not Touch: plan.md, ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md, route-debug files, target-output baselines, unsupported expectations, same-block lookup code, semantic resolver code, control-flow code, store-source publication code, printer/debug code.
Context: Steps 3 and 4 implemented and proved the focused value-home agreement helper, including positive, stale, ambiguous, missing, and null/unavailable rows. No helper behavior changes are expected for Step 5.
Task: Run the supervisor-selected broader validation for the candidate, record pass/fail evidence and closure readiness in `todo.md`, and leave lifecycle decisions to the plan owner.
Proof: (cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
Done When: `todo.md` records the final proof result, remaining out-of-scope rows if any, and whether the selected candidate is ready for plan-owner closure review.
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
- The helper distinguishes "public lookup returned a row" from "structured BIR
  agreement exists"; keep Step 5 validation-only unless a fresh proof exposes a
  real value-home agreement bug.
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
