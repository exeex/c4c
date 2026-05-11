Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Make BIR Validation and Backend Preparation LinkNameId-Authoritative

# Current Packet

## Just Finished

Step 6 reviewer-triggered BIR validation consistency fix is implemented.

`bir::validate()` now permits non-indirect direct calls with an empty raw
`CallInst::callee` when `callee_link_name_id` resolves to a declared function.
The same validation path still rejects unknown `LinkNameId` values, known IDs
without a declared function, and direct calls whose raw declared callee name
disagrees with the ID-authoritative function.

`backend_lir_to_bir_notes_test` covers the new ID-only direct-call validation
case and the rejection cases for unknown IDs, undeclared ID callees, and
ID/raw declared-function mismatches.

## Suggested Next

Continue Step 6 by moving prepared call argument/source-symbol publication off
raw symbol strings where BIR already carries `LinkNameId` metadata, or by
adding a diagnostic/note surface for direct-call ID/raw mismatches if the
supervisor wants explicit reporting beyond verifier rejection.

## Watchouts

- `PreparedCallPlan::direct_callee_name` is still a string field. This packet
  makes its contents semantic-ID-derived when possible, but it does not add a
  prepared `LinkNameId` field.
- Raw-only direct calls remain compatibility-accepted by validation; the stricter
  declared-function check only applies when `callee_link_name_id` is present.
- Dynamic-stack intrinsic detection still intentionally reads raw callee text;
  those synthesized runtime/intrinsic calls keep `callee_link_name_id` invalid.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 3/3 selected tests
passed.
