# Current Packet

Status: Active
Source Idea Path: ideas/open/169_route_local_identity_domain_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce Or Reuse A Route-Local Id Domain

## Just Finished

Step 2 introduced the typed BIR route-local slot id boundary for the selected
BIR local-slot authority family.

Completed work:
- Extended `bir::NameTables` with `SlotNameTable`.
- Added `SlotNameId` fields to `LocalSlot`, `LoadLocalInst`,
  `StoreLocalInst`, local-slot `MemoryAddress` bases, and call sret storage.
- Populated ids in the existing BIR module finalization pass after lowering,
  alongside the existing `BlockLabelId` finalization.
- Updated BIR validation so present slot ids are authoritative and fail closed
  when unknown, duplicated, out of function, or paired with different spelling.
  No-id declarations/references still use legacy spelling fallback.
- Updated prepared stack-layout and call-plan consumers to resolve prepared
  slot ids through BIR slot ids when present, preserving rendered spelling.

Files changed:
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir_validate.cpp`
- `src/backend/bir/lir_to_bir/module.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/stack_layout/analysis.cpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`

## Suggested Next

Step 3 should add focused validation/lookup tests that construct mismatched or
ambiguous BIR local-slot spellings with ids present. The useful cases are:
duplicate `LocalSlot::slot_id`, invalid/unknown ids, load/store ids paired with
the wrong spelling, and local-slot `MemoryAddress` ids that must resolve by id
rather than by fallback text.

## Watchouts

- The original delegated proof target `c4c` is not present in this build tree;
  the accepted proof uses the available compiler target `c4cll`.
- The corrected proof log in `test_after.log` contains the supervisor-selected
  CTest subset after the `c4cll` build.
- Step 3 tests should directly exercise BIR validation; the current packet did
  not add test files.

## Proof

Build/proof results in `test_after.log`:
- `cmake --build build --target c4cll &&`
  `ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepared_printer|backend_cli_dump_bir_is_semantic|backend_cli_dump_prepared_bir_is_prepared|backend_cli_dump_bir_layout_sensitive_aggregate|backend_cli_dump_prepared_bir_local_arg_call_contract|backend_codegen_route_x86_64_local_.*observe_semantic_bir|backend_codegen_route_x86_64_builtin_mem.*local.*observe_semantic_bir)$'`:
  pass, 27/27 tests.
- `git diff --check`: pass.
