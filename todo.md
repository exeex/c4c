Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Validation and Closure Readiness

# Current Packet

## Just Finished

Completed Step 5: broader validation and closure-readiness review for idea
260's `names` same-block lookup candidate.

The selected candidate is complete under this runbook. Step 3 implemented the
shared `prepared_bir_value_name_agreement(...)` helper and wired the selected
same-block scalar/integer lookup plus select-chain source-producer path through
it. Step 4 added the remaining public compatibility proof rows: agreed
`ValueNameId` lookups still resolve, raw-spelling compatibility remains
available only when the prepared id/spelling/result agree, and a prepared id
paired with a drifted BIR producer result fails closed.

## Suggested Next

Request plan-owner closure review for this selected same-block value-name
candidate. Remaining idea 260 candidates should stay open for separate
one-candidate runbooks.

## Watchouts

- Keep this runbook limited to the selected `names` same-block value-name
  lookup candidate.
- Do not treat this activation as approval for `PreparedBirModule` deletion,
  privatization, wrapping, broad aggregate retirement, or another idea 260
  candidate.
- Preserve prepared name tables, existing lookup maps, public aggregate
  compatibility, and current null or unavailable fallback behavior.
- Preserve raw-spelling lookup only as compatibility behavior; do not treat it
  as structured prepared/BIR value-name agreement.
- The completed packet preserved route-debug, target output, baselines,
  unsupported expectations, helper/oracle status names, printer/debug strings,
  value-home lookup, semantic resolver API, control-flow, and store-source
  publication behavior.
- Residual out-of-scope surfaces remain route-debug, target output, baselines,
  unsupported expectations, helper/oracle status names, printer/debug strings,
  value-home lookup, semantic resolver API, control-flow, store-source
  publication behavior, and every other idea 260 candidate.
- Do not rewrite route-debug, target output, baselines, unsupported
  expectations, helper/oracle status names, printer/debug strings, value-home
  lookup, semantic resolver API, control-flow, or store-source publication
  behavior to claim progress.

## Proof

Focused Step 4 proof:

```bash
(cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
```

Result: passed. `test_after.log` shows `backend_prepared_lookup_helper` passed.
`git diff --check -- tests/backend/bir/backend_prepared_lookup_helper_test.cpp
todo.md` passed.

Broader Step 5 proof:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^backend_' -j --output-on-failure) > test_before.log 2>&1
```

Result: passed before and after this `todo.md` closure-readiness update.
`test_before.log` and `test_after.log` both show 180/180 backend tests passed.
The regression guard comparison reported no new failures.
