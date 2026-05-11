# Current Packet

Status: Active
Source Idea Path: ideas/open/169_route_local_identity_domain_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Route-Local Validation Or Lookup Tests

## Just Finished

Step 3 focused validation tests completed: `backend_lir_to_bir_notes_test.cpp`
now proves BIR local-slot ids are semantic authority and fail closed before
falling back to spelling on id/name mismatches.

Completed work:
- Added direct BIR verifier coverage for duplicate `LocalSlot::slot_id` values
  in one function.
- Added `LoadLocalInst` and `StoreLocalInst` mismatch cases where a present
  `slot_id` is paired with a different `slot_name`.
- Added a local-slot `MemoryAddress::base_slot_id` mismatch case where the
  address spelling points at a different declared local slot.
- Preserved no-id compatibility by proving raw spelling still validates when an
  existing declared local slot is named.

Files changed:
- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `todo.md`

## Suggested Next

Step 4 can proceed by removing or tightening any remaining route-local string
lookup fallback that is now covered by semantic id validation.

## Watchouts

- This packet did not require production changes; the existing Step 2
  implementation already rejects the tested mismatch classes.
- The new tests live inside `backend_lir_to_bir_notes` because that target
  already links `bir_validate.cpp` and is part of the delegated proof subset.
- `review/168_step4_hir_bridge_route_review.md` remains unrelated dirty state
  if present.

## Proof

Build/proof results:
- `cmake --build build -j`: pass.
- `ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_lir_to_bir_notes|backend_prepared_printer|backend_cli_dump_bir_is_semantic|backend_cli_dump_prepared_bir_is_prepared|backend_cli_dump_bir_layout_sensitive_aggregate|backend_cli_dump_prepared_bir_local_arg_call_contract|backend_codegen_route_x86_64_local_.*observe_semantic_bir|backend_codegen_route_x86_64_builtin_mem.*local.*observe_semantic_bir)$' > test_after.log 2>&1`:
  pass, 28/28 tests, log path `test_after.log`.
- `git diff --check`: pass.
