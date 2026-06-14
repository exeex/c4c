Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Fail-Closed Proof Rows

# Current Packet

## Just Finished

Completed Step 4: added the remaining public compatibility proof rows for idea
260's `names` same-block lookup candidate.

The existing Step 3 rows already covered the main positive scalar,
integer-constant, and select-chain paths plus fail-closed behavior for unnamed,
empty, missing prepared id, stale producer, wrong type, duplicate/unknown
producer rows, missing maps, and prepared/BIR name drift through the `bir::Value`
surface. Step 4 adds focused `ValueNameId` overload coverage: agreed prepared
ids still resolve, raw-spelling compatibility remains available only when the
prepared id/spelling/result agree, and the public `ValueNameId` overload fails
closed when a prepared id is paired with a drifted BIR producer result.

## Suggested Next

Execute Step 5: run broader validation and decide whether this selected
same-block value-name candidate is ready for plan-owner closure review.

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
- The Step 4 packet should preserve route-debug, target output, baselines,
  unsupported expectations, helper/oracle status names, printer/debug strings,
  value-home lookup, semantic resolver API, control-flow, and store-source
  publication behavior.
- Step 4 preserved helper behavior and only added proof rows. Residual
  out-of-scope surfaces remain route-debug, target output, baselines,
  unsupported expectations, helper/oracle status names, printer/debug strings,
  value-home lookup, semantic resolver API, control-flow, and store-source
  publication behavior.
- Do not rewrite route-debug, target output, baselines, unsupported
  expectations, helper/oracle status names, printer/debug strings, value-home
  lookup, semantic resolver API, control-flow, or store-source publication
  behavior to claim progress.

## Proof

Step 4 proof:

```bash
(cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
```

Result: passed. `test_after.log` shows `backend_prepared_lookup_helper` passed.
`git diff --check -- tests/backend/bir/backend_prepared_lookup_helper_test.cpp
todo.md` passed.
