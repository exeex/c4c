Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Make BIR Validation and Backend Preparation LinkNameId-Authoritative

# Current Packet

## Just Finished

Step 6 prepared-call direct-callee slice is implemented.

`populate_call_plans()` now resolves direct callees through
`CallInst::callee_link_name_id` before raw `callee` spelling. Valid IDs publish
the semantic link-name spelling into `PreparedCallPlan::direct_callee_name` and
wrapper classification looks up same-module/extern declarations by
`Function::link_name_id`. Raw spelling remains the compatibility fallback only
when no callee ID is present.

Covered callee ID/display mismatches now fail closed for prepared call plans:
they do not publish the raw spelling as the direct callee and do not classify
the call as a different same-module or direct extern target by string.
`backend_prepare_frame_stack_call_contract_test` covers ID-only direct calls,
raw-only compatibility calls, and mismatch fail-closed behavior.
`backend_prepared_printer_test` now proves prepared dumps print the semantic ID
callee spelling even when the raw direct-callee spelling is absent.

## Suggested Next

Continue Step 6 by moving prepared call argument/source-symbol publication off
raw symbol strings where BIR already carries `LinkNameId` metadata, or by adding
a diagnostic/note surface for fail-closed prepared-call mismatches if the
supervisor wants explicit reporting instead of silent omission.

## Watchouts

- `PreparedCallPlan::direct_callee_name` is still a string field. This packet
  makes its contents semantic-ID-derived when possible, but it does not add a
  prepared `LinkNameId` field.
- Mismatched direct calls use the existing `Indirect` wrapper enum as the
  fail-closed prepared state while keeping `is_indirect == false` and no
  indirect callee plan. There is still no dedicated invalid/diagnostic wrapper
  channel.
- Dynamic-stack intrinsic detection still intentionally reads raw callee text;
  those synthesized runtime/intrinsic calls keep `callee_link_name_id` invalid.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_prepare_liveness_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 3/3 selected tests
passed.
