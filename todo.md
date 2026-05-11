# Current Packet

Status: Active
Source Idea Path: ideas/open/169_route_local_identity_domain_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce Or Reuse A Route-Local Id Domain

## Just Finished

Step 2 integration fix completed: `backend_prepare_stack_layout_test.cpp` now
passes a `bir::NameTables` to the updated
`prepare::stack_layout::collect_function_stack_objects` signature.

Completed work:
- Added a test-local helper that interns direct fixture `LocalSlot::slot_id`
  values through `bir::NameTables::slot_names`.
- Updated the four direct stack-object collection helpers to pass the BIR name
  table into `collect_function_stack_objects`.
- Preserved the existing output spelling and assertions while exercising the
  new slot-id authority path for local-slot fixtures.

Files changed:
- `tests/backend/backend_prepare_stack_layout_test.cpp`
- `todo.md`

## Suggested Next

Step 3 can proceed with focused validation/lookup tests for mismatched or
ambiguous BIR local-slot spellings with ids present.

## Watchouts

- This packet only repairs the missed test integration with the new
  `collect_function_stack_objects` signature; it does not add the Step 3
  validation cases.
- `review/168_step4_hir_bridge_route_review.md` remains unrelated dirty state
  if present.

## Proof

Build/proof results:
- `cmake --build build -j`: pass.
- `ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_lir_to_bir_notes|backend_prepared_printer|backend_cli_dump_bir_is_semantic|backend_cli_dump_prepared_bir_is_prepared|backend_cli_dump_bir_layout_sensitive_aggregate|backend_cli_dump_prepared_bir_local_arg_call_contract|backend_codegen_route_x86_64_local_.*observe_semantic_bir|backend_codegen_route_x86_64_builtin_mem.*local.*observe_semantic_bir)$' > test_after.log 2>&1`:
  pass, 28/28 tests, log path `test_after.log`.
- `git diff --check`: pass.
