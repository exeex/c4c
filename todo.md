Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Narrow Value-Name Bridge

# Current Packet

## Just Finished

Completed Step 3: implemented the shared value-name agreement bridge for idea
260's `names` same-block lookup candidate.

- `src/backend/prealloc/lookup_agreement.hpp`
- `src/backend/prealloc/lookup_agreement.cpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/select_chain_lookups.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

The new `prepared_bir_value_name_agreement(...)` helper accepts only non-empty
named BIR values whose prepared `ValueNameId`, source-producer map row, same
prepared block label, before-cutoff producer instruction, result pointer,
result type, and prepared value-name spelling agree. The selected prepared
same-block scalar/integer lookup path and select-chain source-producer path now
share that boundary. Focused rows prove positive scalar, integer-constant, and
select-chain agreement plus fail-closed behavior for unnamed, empty, missing
prepared id, stale producer, wrong type, duplicate/unknown producer rows,
missing maps, and prepared/BIR name drift.

## Suggested Next

Execute Step 4: add any remaining fail-closed proof rows the supervisor wants
around public compatibility boundaries, especially raw-spelling compatibility
that must not be counted as structured agreement.

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
- Step 3 covered the main positive and fail-closed rows through public prepared
  scalar, integer-constant, and select-chain APIs. Step 4 should focus on any
  remaining public compatibility surfaces rather than reworking helper logic.
- Do not rewrite route-debug, target output, baselines, unsupported
  expectations, helper/oracle status names, printer/debug strings, value-home
  lookup, semantic resolver API, control-flow, or store-source publication
  behavior to claim progress.

## Proof

Step 3 implementation proof:

```bash
(cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
```

Result: passed. `test_after.log` shows `backend_prepared_lookup_helper` passed.
`git diff --check` passed for the owned implementation/test files before this
`todo.md` update.
