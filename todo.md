Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate and Tighten Compatibility Boundaries

# Current Packet

## Just Finished

Step 6 Validate and Tighten Compatibility Boundaries repaired materialized
compare-join same-module global handling so BIR
`LoadGlobalInst::global_name_id` and `MemoryAddress::base_link_name_id` are
resolved through BIR name tables, semantic spellings are interned into
`PreparedNameTables`, and downstream prepared refs use prepared IDs with
stable prepared-table spellings.

## Suggested Next

Supervisor should decide whether to continue with the next Step 6 tightening
packet or escalate to broader backend validation before a milestone decision.

## Watchouts

- Raw same-module global names remain accepted only for compatibility contracts
  where the prepared reference has an invalid `LinkNameId`.
- Legacy overloads still preserve existing callers by importing prepared names
  into a BIR-name view owned by the returned context; the materialized
  render-contract path now passes `module.names` explicitly.
- The focused ownership test now checks that classified LinkNameId-backed
  spellings point into prepared name storage rather than BIR name storage.
- The untracked `review/` artifacts were not touched.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepare_frame_stack_call_contract_test backend_prepare_structured_context_test backend_prepare_authoritative_join_ownership_test frontend_hir_tests && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepare_frame_stack_call_contract|backend_prepare_structured_context|backend_prepare_authoritative_join_ownership|frontend_hir_tests)$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 5/5 selected tests
passed.
