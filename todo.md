# Current Packet

Status: Active
Source Idea Path: ideas/open/749_lir_selected_memcpy_current_function_pointer_object_lifetime_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define bounded current-function pointer and object authority

## Just Finished

- Plan Step 1 complete: added the opt-in selected-pair current-function
  pointer-definition authority with typed pointer value IDs, distinct local
  object IDs, one `LirFunction` owner, and live-at-selected-site facts. The
  LIR verifier accepts only a resolvable same-owner live pair and rejects
  invalid, foreign, duplicate-object, and non-pointer/dead definitions without
  consulting presentation text.

## Suggested Next

- Execute Plan Step 2 only: populate this authority for the selected fixed
  aggregate byval parameter materialization at `lvalue.cpp:279-281`, then add
  its authoritative-use rejection boundaries. Do not alter unrelated pointer
  families or memcpy schema/behavior beyond that step's explicit scope.

## Watchouts

- `LirFunction::selected_memcpy_pointer_authority` is intentionally the only
  carrier: both definitions need `LirValueId`, pointer `LirTypeRef`, distinct
  `LirObjectId`, the current function's `LinkNameId`, and live-at-site state.
  It remains optional until Step 2; no memcpy field has been populated.

## Proof

- Supervisor proof passed: `cmake --preset default`, `cmake --build --preset
  default`, and `ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed 5/5, including `backend_lir_selected_pointer_authority`. Regression
  guard passed against canonical `test_before.log` 4/4 with `test_after.log`
  5/5 and no new failing tests.
