Status: Active
Source Idea Path: ideas/open/302_rv64_prepared_emitter_context_interface_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Shared Prepared Lookup Surfaces

# Current Packet

## Just Finished

Step 1 completed the prepared RV64 owner-header inventory without implementation
edits.

Header surface classification:
- `prepared_scalar_emit.hpp`: mixes owner emission APIs with shared lookup
  utilities. Owner APIs are `SimpleCompare`,
  `emit_riscv_simple_compare_branch`, `emit_riscv_simple_cast`,
  `emit_riscv_simple_select`, `emit_riscv_simple_prepared_pointer_add`,
  `emit_riscv_simple_binary`, and `emit_riscv_simple_return`. Shared lookup
  utilities are `prepared_value_home_for`, `prepared_register_for_value`,
  `prepared_pointer_register_for_value`,
  `prepared_register_for_value_name_id`, `emit_move_to_register`, and
  `has_frame_slot_address_materialization_at`.
- `prepared_local_memory_emit.hpp`: owner APIs are
  `emit_riscv_simple_store_local` and `emit_riscv_simple_load_local`; frame-ish
  support utilities are `emit_i32_load_from_stack_offset` and
  `emit_i32_store_to_stack_offset`.
- `prepared_frame_emit.hpp`: frame/support utilities only:
  immediate/offset fit checks, stack alignment helpers, saved-register frame
  lookup, label spelling, target spelling, and `simple_frame_slot_sp_offset_for`
  overloads.
- `prepared_call_emit.hpp`: owner API only,
  `emit_riscv_simple_call`, but it consumes scalar's shared move helper.
- `prepared_function_emit.hpp`: owner orchestration API only,
  `append_simple_prepared_bir_function_asm`.
- `prepared_edge_publication_emit.hpp`: no exported helpers; compatibility API
  remains in `emit.hpp`.
- `prepared_module_emit.hpp`: owner API only, `emit_prepared_module_text`.

Repeated bundles found:
- `PreparedNameTables&` plus `PreparedFunctionLookups*` appears in scalar
  value-home/register helpers, scalar emission APIs, call emission,
  local-memory emission, and function orchestration call sites.
- `PreparedFunctionLookups*` plus `BlockLabelId` plus instruction index appears
  in frame-slot address-materialization checks and local-memory access helpers.
- The wider tuple `PreparedNameTables&`, `PreparedFunctionLookups*`,
  `BlockLabelId`, and instruction index appears in
  `emit_riscv_simple_prepared_pointer_add`, both local-memory owner entry
  points, and function-owner dispatch.

## Suggested Next

Delegate the first implementation packet for Step 2: introduce a narrow
`prepared_emit_context.{hpp,cpp}` support surface and move the scalar-exported
prepared value/register lookup declarations and definitions there first:
`prepared_value_home_for`, `prepared_register_for_value`,
`prepared_pointer_register_for_value`, `prepared_register_for_value_name_id`,
and `has_frame_slot_address_materialization_at`. Keep behavior unchanged and
update includes/build metadata only as needed for that extraction.

## Watchouts

- Keep this idea behavior-preserving; do not add RV64 runtime cases or broaden
  backend capability.
- Do not weaken tests, expectations, unsupported handling, or qemu runtime
  coverage.
- Avoid creating a central support file that becomes a miscellaneous helper
  pile.
- `emit_move_to_register` is shared by scalar, call, and local-memory, but it
  emits assembly and depends on stack-load helpers; leave it behind until the
  lookup-only support surface exists or move it only if the supervisor explicitly
  accepts that broader coupling.
- `emit_i32_load_from_stack_offset`, `emit_i32_store_to_stack_offset`, and
  `simple_frame_slot_sp_offset_for` are better handled by the later
  frame/local-memory boundary step, not the first lookup extraction.

## Proof

No build or test command was run for this inventory-only packet. Use this proof
command for the next implementation packet:
`cmake --build --preset default && ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`.
