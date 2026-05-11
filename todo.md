Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Make BIR Validation and Backend Preparation LinkNameId-Authoritative

# Current Packet

## Just Finished

Step 6 prepared call argument/source-symbol publication is now
`LinkNameId`-authoritative when BIR values carry valid
`pointer_symbol_link_name_id` metadata.

`PreparedCallArgumentPlan` publishes an optional prepared
`source_symbol_name_id` alongside the existing compatibility string. Prepared
call-plan construction resolves symbol pointer arguments through BIR
`LinkNameId` metadata first, maps valid symbols into the prepared link-name
table, keeps raw `@symbol` compatibility when no metadata exists, and fails
closed instead of publishing a symbol address when raw display text disagrees
with the semantic ID.

`backend_prepare_frame_stack_call_contract_test` covers ID-derived symbol
publication from an empty raw display name and stale raw/ID mismatch rejection.
`backend_prepared_printer_test` covers the ID-backed prepared-printer detail
surface while preserving the existing `source_symbol=@...` output.

## Suggested Next

Continue Step 6 by adding any remaining diagnostic/note surface the supervisor
wants for direct-call or symbol-pointer ID/raw mismatches beyond verifier and
prepared-plan fail-closed behavior, or move to supervisor review if that
surface is intentionally out of scope for this plan.

## Watchouts

- `PreparedCallPlan::direct_callee_name` is still a string field. This packet
  makes its contents semantic-ID-derived when possible, but it does not add a
  prepared `LinkNameId` field.
- `PreparedCallArgumentPlan::source_symbol_name` remains for compatibility with
  current consumers and text output; `source_symbol_name_id` is the authoritative
  field when present.
- Raw-only symbol arguments remain compatibility-accepted when
  `pointer_symbol_link_name_id` is invalid.
- Dynamic-stack intrinsic detection still intentionally reads raw callee text;
  those synthesized runtime/intrinsic calls keep `callee_link_name_id` invalid.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_prepare_liveness_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prepare_liveness)$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 3/3 selected tests
passed.
