Status: Active
Source Idea Path: ideas/open/537_rv64_object_local_memory_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map the local-memory boundary

# Current Packet

## Just Finished

Step 1 from `plan.md` mapped the RV64 object-route local-memory helper boundary without editing implementation files.

Move set for Step 2:
- Encoded local load/store entry points: `fragment_for_prepared_store_local` and `fragment_for_prepared_load_local`.
- Direct local-memory addressability helpers used by those fragments and matching diagnostics: `prepared_frame_slot_absolute_byte_offset`, `prepared_pointer_value_base_offset`, `prepared_byval_stack_slot_pointer_access_offset`, `prepared_sret_stack_slot_pointer_access`, and `PreparedSretStackPointerAccess`.
- Local address materialization support used only to store frame-slot pointer values: `prepared_frame_slot_address_materialization_offset` and `append_rv64_materialize_or_move_store_value`.
- Local type-size helper `rv64_local_memory_size_for_type`, but only if Step 2 keeps or exports a compatibility declaration for its current diagnostic/param-home users.

Already owned by `prepared_local_memory_emit.*`:
- Textual/simple local load/store APIs: `emit_riscv_simple_store_local` and `emit_riscv_simple_load_local`.
- Private simple-route helpers for frame-slot accesses, pointer-value base-plus-offset accesses, pointer loads/stores, simple frame-slot stack offsets, pointer address materializations, and textual pointer-base register loading.

Parked set:
- `fragment_for_prepared_frame_address_materialization` stays in `object_emission.cpp` for now as the object-route Binary/Add dispatch wrapper for prepared frame-slot address materialization.
- `prepared_direct_global_materialization_symbol`, `prepared_address_materialization_symbol`, and `fragment_for_prepared_symbol_address_materialization` stay parked because they own global/string/object fixup materialization.
- `rv64_scalar_memory_size_for_type` stays parked because AST callers span calls, moves, casts, selects, param-home diagnostics, and other non-local-memory object-route families.
- `prepared_frame_slot_absolute_offset` is currently unused by direct AST caller query; leave for Step 3 prune/park review rather than moving it in Step 2.
- `prepared_memory_access_for_instruction` stays parked as an object-route lookup wrapper used at dispatch/diagnostic call sites.
- Generic encoded helpers such as register lookup, stack load/store emitters, move materialization, floating store preparation, temporary register choice, and pcrel/object fixup builders stay parked unless a public shared helper already exists; Step 2 should depend on declarations/wrappers instead of widening into those families.

Include/declaration prerequisites for Step 2:
- `prepared_local_memory_emit.hpp` needs `object_emission.hpp` or an equivalent declaration path for `RiscvEncodedFragment`.
- `prepared_local_memory_emit.hpp` needs public declarations for the encoded local store/load helpers if `object_emission.cpp` will call them after extraction.
- `prepared_local_memory_emit.cpp` will need access to prepared stack layout/name/lookups, BIR load/store types, frame helper functions, and any encoded helper declarations used by the moved fragments.
- If diagnostics in `object_emission.cpp` continue to call moved addressability helpers, expose narrow declarations for those helpers or leave diagnostic-only wrappers in `object_emission.cpp`.

## Suggested Next

Execute Step 2 from `plan.md`: extract the encoded local-memory helper family into `prepared_local_memory_emit.*` while preserving the parked object/global/data boundaries.

## Watchouts

- Do not move global symbol materialization, prepared data-object emission, relocation handling, ELF writing, or module assembly.
- Do not change local-array semantics, pointer provenance, prepared memory facts, tests, unsupported markers, or gcc_torture expectations.
- `prepared_local_memory_emit.cpp` already has textual helpers that admit direct-global and string-constant pointer materializations for simple local stores/loads; do not treat that as permission to move the encoded object fixup symbol helpers in this runbook.
- Moving `fragment_for_prepared_store_local` pulls store-value frame-slot address materialization support; keep that scoped to local pointer values and do not pull in symbol address materialization.
- Diagnostics currently call the local addressability predicates directly, so Step 2 should preserve those diagnostics without weakening unsupported messages.

## Proof

No build required for mapping-only Step 1; no `test_after.log` written.

Exact Step 2 validation command from `plan.md`:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_prepared_local_array|obj_runtime_rv64_local_temp|obj_runtime_rv64_large_fixed_frame_slot_access|rv64_runtime_riscv64_pointer_to_pointer_local_address)'
```
