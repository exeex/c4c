Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Make BIR Validation and Backend Preparation LinkNameId-Authoritative

# Current Packet

## Just Finished

Step 6 BIR direct-call validation is now explicitly
`LinkNameId`-authoritative for ID-bearing non-indirect calls.

The verifier resolves `CallInst::callee_link_name_id` through the BIR
link-name table and declared function IDs, accepts an empty raw callee when the
ID names a declared function, rejects raw/ID mismatches against declared
function names, and does not fall back to a matching raw callee spelling when
the semantic ID names no declared function.

`backend_lir_to_bir_notes_test` now covers the no-raw-fallback direct-call case
where `callee = "actual_callee"` but `callee_link_name_id` resolves to an
undeclared `missing_callee` ID.

## Suggested Next

Supervisor review or the next Step 6 packet can decide whether any remaining
diagnostic/note surface is needed beyond validator fail-closed behavior and the
prepared-call behavior already landed.

## Watchouts

- Raw-only direct calls remain compatibility-accepted when
  `callee_link_name_id` is invalid.
- Runtime/intrinsic placeholder calls still intentionally keep
  `callee_link_name_id` invalid and validate through raw compatibility spelling.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepare_frame_stack_call_contract_test && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepare_frame_stack_call_contract)$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 2/2 selected tests
passed.
