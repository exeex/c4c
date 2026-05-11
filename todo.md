Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Make BIR Validation and Backend Preparation LinkNameId-Authoritative

# Current Packet

## Just Finished

Step 6 backend-preparation stack-layout addressing slice is implemented.

`build_direct_symbol_backed_address()` now consumes covered
`MemoryAddress::base_link_name_id` and `LoadGlobalInst`/`StoreGlobalInst`
`global_name_id` metadata before raw display spellings. Valid IDs resolve
through the BIR link-name table into the prepared link-name table; raw spelling
remains a compatibility fallback only when the covered ID is absent. Covered
ID/display mismatches fail closed by not publishing a direct-symbol prepared
memory access.

`backend_prepare_stack_layout_test` now proves ID-only global addressing
resolves to the canonical `LinkNameId` spelling, drifted raw global spelling
cannot override that ID on the stack-layout prepared-addressing route, and
raw-only compatibility global stores still resolve by spelling.

## Suggested Next

Continue Step 6 by moving another prepared/backend route off raw display names,
preferably a call-plan source-symbol or storage-plan symbol path that already
receives `LinkNameId` metadata from BIR.

## Watchouts

- This packet fail-closes mismatches by omitting the prepared direct-symbol
  access rather than emitting a diagnostic. That matches the current
  stack-layout addressing API shape; a later validation/reporting packet may
  want an explicit prepare note or error surface.
- String constants still intentionally use raw spellings because BIR string
  constants do not carry semantic `LinkNameId`.
- Local-slot pointer-value addressing still records route-local value names,
  not module-level link-name authority.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_prepare_stack_layout_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 3/3 selected tests
passed.
