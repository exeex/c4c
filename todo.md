Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Validation and Closure Readiness

# Current Packet

## Just Finished

Step 5 broader validation and closure-readiness review complete for the
`names` value-home lookup candidate.

- Broader validation exposed a compatibility regression in
  `backend_aarch64_branch_control_lowering`: a stale prepared producer mismatch
  expected BIR fallback, but value-home agreement rejected copied prepared
  value-home lookup rows solely because the indexed home pointer came from an
  equivalent snapshot.
- Changed `prepared_bir_value_home_agreement(...)` to keep the prepared
  `ValueNameId` and `PreparedValueId` index checks, then compare the indexed
  home row semantically with the unique backing `PreparedValueLocationFunction`
  row instead of requiring raw pointer equality.
- Added focused helper-test coverage for accepting an equivalent copied home
  index and rejecting a copied index with the same value id but stale home
  payload.
- Preserved fail-closed behavior for stale value ids, missing indexes, missing
  backing homes, duplicate/conflicting names or ids, and conflicting regalloc
  rows.
- Reran the broader backend subset after the fix; it passed and the selected
  value-home candidate is ready for plan-owner closure review.

## Suggested Next

Request plan-owner closure review for this selected value-home lookup
candidate. Remaining idea 260 candidates should stay open for separate
one-candidate runbooks.

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
  agreement exists"; this packet fixed one real value-home agreement bug found
  by broader validation without relaxing the required prepared value-name and
  prepared value-id agreement.
- Direct prepared lookup compatibility intentionally keeps first-emplace
  behavior for duplicates. Do not reinterpret that compatibility behavior as
  BIR agreement.
- `src/backend/prealloc/prepared_lookups.cpp` was owned for Step 3 but did not
  need changes because the selected public BIR-value entrypoint is inline in
  `value_locations.hpp`.

## Proof

Ran:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering)$' --output-on-failure) > test_after.log 2>&1
```

Result: passed. `test_after.log` shows `100% tests passed, 0 tests failed out
of 2`.

Broader Step 5 proof:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^backend_' -j --output-on-failure) > test_before.log 2>&1
```

Result: passed before and after this closure-readiness update.
`test_before.log` and `test_after.log` both show 180/180 backend tests passed.
The regression guard comparison reported no new failures.
