Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate and Tighten Compatibility Boundaries

# Current Packet

## Just Finished

Step 6 Validate and Tighten Compatibility Boundaries added focused
`backend_lir_to_bir_notes` verifier coverage proving global load/store
validation is LinkNameId-authoritative: ID-only declared globals validate,
unknown LinkNameIds fail before raw-name lookup, declared-but-undeclared
LinkNameIds fail closed even when the raw name matches, and raw-only global
load/store compatibility remains valid only when no LinkNameId is present.

## Suggested Next

Supervisor should decide whether this completes the Step 6 compatibility-boundary
tightening or whether another focused verifier boundary needs coverage before
broader backend validation.

## Watchouts

- No production validator change was needed; existing global load/store
  validation already uses `LinkNameId` as authoritative when present.
- Raw same-module global names remain accepted only for compatibility contracts
  where the BIR reference has an invalid `LinkNameId`.
- The untracked `review/` artifacts were not touched.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepare_frame_stack_call_contract_test backend_prepare_stack_layout_test backend_prepare_authoritative_join_ownership_test backend_prepare_structured_context_test frontend_hir_tests && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepare_frame_stack_call_contract|backend_prepare_stack_layout|backend_prepare_authoritative_join_ownership|backend_prepare_structured_context|frontend_hir_tests)$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 6/6 selected tests
passed.
