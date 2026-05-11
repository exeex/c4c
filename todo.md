Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate and Tighten Compatibility Boundaries

# Current Packet

## Just Finished

Step 6 Validate and Tighten Compatibility Boundaries added the remaining focused
`backend_lir_to_bir_notes` verifier coverage for BIR initializer-symbol
LinkNameId boundaries. Raw-only initializer symbols remain compatibility-valid
only without LinkNameId, declared global/function LinkNameIds validate even when
the retained display spelling drifts, unknown LinkNameIds reject before raw-name
lookup, and known-but-undeclared LinkNameIds fail closed instead of falling back
to matching raw spelling.

## Suggested Next

Supervisor should treat Step 6 as ready for broader proof or lifecycle closure
review unless reviewer scrutiny identifies another compatibility boundary.

## Watchouts

- No production validator change was needed; existing global load/store
  validation and initializer-symbol validation already use `LinkNameId` as
  authoritative when present.
- Raw same-module global names and initializer-symbol spellings remain accepted
  only for compatibility contracts where the BIR reference has an invalid
  `LinkNameId`.
- The untracked `review/` artifacts were not touched.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepare_frame_stack_call_contract_test backend_prepare_stack_layout_test backend_prepare_authoritative_join_ownership_test backend_prepare_structured_context_test frontend_hir_tests && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepare_frame_stack_call_contract|backend_prepare_stack_layout|backend_prepare_authoritative_join_ownership|backend_prepare_structured_context|frontend_hir_tests)$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 6/6 selected tests
passed.
