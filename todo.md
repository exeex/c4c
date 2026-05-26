Status: Active
Source Idea Path: ideas/open/12_dispatch_value_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Materialization Helpers and Clients

# Current Packet

## Just Finished

Step 1 audit completed for the public surface in
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.hpp`.
AST-backed checks confirmed the header declarations, implementation sites in
`dispatch_value_materialization.cpp`, TU-local call graph edges, and dispatch
caller sites; text search confirmed direct include/call clients across the
AArch64 codegen helpers.

| Helper | Implementation | Direct clients | Owner classification | Stay vs move | First safe move packet | Focused proof candidates |
| --- | --- | --- | --- | --- | --- | --- |
| `emit_fp_immediate_to_register` | `dispatch_value_materialization.cpp:73` | Internal only: `emit_fp_value_to_register` | Leaf constant/FP bitcast materializer | Move with FP value materialization; no dispatch ownership | FP materialization extraction after scalar publication helper remains callable | `backend_aarch64_machine_printer`, `backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result`, `backend_aarch64_return_lowering` |
| `emit_fp_value_to_register` | `dispatch_value_materialization.cpp:116` | `memory_store_sources.cpp`; `dispatch_edge_copies.cpp`; internal recursion; `emit_value_publication_to_register`; `lower_scalar_fp_binary_publication_to_prepared_register` | Recursive FP value materializer shared by stores, edge copies, and prepared FP publication | Move to value-materialization owner with a stable header; keep callable from store/edge-copy clients | FP materialization packet with `emit_fp_immediate_to_register` and FP prepared-register lowering | `backend_aarch64_machine_printer`, `backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result`, `backend_aarch64_return_lowering` |
| `emit_prepared_global_symbol_load_to_register` | `dispatch_value_materialization.cpp:35` | Internal only: `emit_value_publication_to_register` | Prepared global-symbol load materializer | Move with generic scalar value publication, not as a standalone dispatch hook | Scalar publication helper extraction | `backend_aarch64_return_lowering`, `backend_aarch64_machine_printer`, `backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call` |
| `emit_prepared_va_list_field_load_to_register` | `dispatch_value_materialization.cpp:413` | `dispatch_edge_copies.cpp`; internal: `emit_value_publication_to_register` | Prepared va_list field load materializer | Move with scalar value publication; edge-copy client should include the new owner header | Scalar publication helper extraction | `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`, `backend_aarch64_return_lowering`, `backend_aarch64_machine_printer` |
| `emit_prepared_pointer_value_load_to_register` | `dispatch_value_materialization.cpp:432` | Internal: `emit_value_publication_to_register`, `lower_stack_homed_pointer_value_load_publication` | Pointer-base prepared memory-load materializer | Move with prepared pointer memory value publication, but keep before dispatch route hooks that wrap it | Pointer-value load publication packet | `backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value`, `backend_aarch64_call_boundary_owner`, `backend_aarch64_machine_printer` |
| `emit_prepared_value_home_to_register` | `dispatch_value_materialization.cpp:512` | `dispatch_edge_copies.cpp`; `comparison_branch_fusion.hpp` hook; internal scalar publication plan helpers and `emit_value_publication_to_register` | Prepared value-home read materializer | Move early with scalar publication utilities; it is the cleanest first ownership cut because clients already call it directly | First safe move packet: extract value-home read/publication helpers behind the same public declarations | `backend_aarch64_instruction_dispatch`, `backend_aarch64_branch_control_lowering`, `backend_aarch64_return_lowering`, `backend_aarch64_machine_printer` |
| `emit_value_publication_to_register` | `dispatch_value_materialization.cpp:647` | `calls_dispatch_bridge.cpp`; `memory_store_sources.cpp`; `dispatch_edge_copies.cpp`; `dispatch_publication.cpp`; `comparison_branch_fusion.hpp` hook; internal recursion and lowering helpers | Central scalar value publication/materialization authority | Stay public during decomposition; move only after leaf helpers are extracted because it fans into producers, memory, branch fusion, edge copies, calls, and store sources | Later scalar publication authority packet after value-home and FP leaf moves | `backend_aarch64_instruction_dispatch`, `backend_aarch64_return_lowering`, `backend_aarch64_call_boundary_owner`, `backend_aarch64_branch_control_lowering`, `backend_aarch64_machine_printer` |
| `lower_local_slot_address_publication` | `dispatch_value_materialization.cpp:1201` | `dispatch.cpp` via `dispatch_prepared_block` | Dispatch route hook for local-slot address publication | Move to address-publication owner, not generic value publication | Address publication hook extraction after value-home helper is stable | `backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address`, `backend_aarch64_instruction_dispatch`, `backend_aarch64_machine_printer` |
| `lower_stack_homed_pointer_value_load_publication` | `dispatch_value_materialization.cpp:1259` | `dispatch.cpp` via `dispatch_prepared_block` | Dispatch route hook for pointer-value load publication into prepared homes | Move with pointer-value load publication; depends on `emit_prepared_pointer_value_load_to_register` | Pointer-value load publication packet | `backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value`, `backend_aarch64_call_boundary_owner`, `backend_aarch64_instruction_dispatch` |
| `make_load_global_got_materialization_instruction` | `dispatch_value_materialization.cpp:1365` | `dispatch.cpp` via `dispatch_prepared_block` | Dispatch route hook for GOT-required load-global materialization | Move to global/address materialization owner, not scalar publication core | Global/GOT materialization hook extraction | `backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call`, `backend_aarch64_return_lowering`, `backend_aarch64_machine_printer` |
| `lower_scalar_mul_with_distinct_rhs_scratch` | `dispatch_value_materialization.cpp:1442` | `dispatch.cpp` via `dispatch_prepared_block`; internal calls to `emit_value_publication_to_register` | Scalar ALU special lowering route with scratch conflict handling | Move to ALU/prepared scalar publication owner after publication helper stays available | Scalar ALU publication hook extraction | `backend_aarch64_instruction_dispatch`, `backend_aarch64_machine_printer`, `backend_aarch64_scalar_alu_records`, `backend_aarch64_prepared_scalar_alu_records` |
| `lower_scalar_cast_publication_to_prepared_register` | `dispatch_value_materialization.cpp:1711` | `dispatch.cpp` via `dispatch_prepared_block`; internal calls to `emit_value_publication_to_register` | Prepared scalar cast/FP compare-to-register publication route | Move to cast/prepared scalar publication owner after value publication helper is stable | Cast publication hook extraction | `backend_aarch64_scalar_cast_records`, `backend_aarch64_prepared_scalar_cast_records`, `backend_aarch64_instruction_dispatch`, `backend_aarch64_machine_printer` |
| `lower_scalar_fp_binary_publication_to_prepared_register` | `dispatch_value_materialization.cpp:1799` | `dispatch.cpp` via `dispatch_prepared_block`; internal call to `emit_fp_value_to_register` | Prepared FP ALU-to-register publication route | Move with FP materialization/FP ops owner | FP materialization packet with `emit_fp_value_to_register` | `backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result`, `backend_aarch64_machine_printer`, `backend_aarch64_instruction_dispatch` |
| `lower_scalar_cast_publication_to_prepared_stack` | `dispatch_value_materialization.cpp:1873` | `dispatch.cpp` via `dispatch_prepared_block`; internal calls to `emit_value_publication_to_register` | Prepared scalar cast-to-stack publication route | Move to cast/prepared scalar publication owner after scalar value publication helper is stable | Cast publication hook extraction | `backend_aarch64_scalar_cast_records`, `backend_aarch64_prepared_scalar_cast_records`, `backend_aarch64_machine_printer`, `backend_aarch64_instruction_dispatch` |

## Suggested Next

First move packet: extract the prepared value-home read/publication leaf helper
surface while preserving existing public names. Own
`dispatch_value_materialization.hpp/.cpp` plus the new target owner file only;
move `emit_prepared_value_home_to_register` and the private
`emit_prepared_scalar_publication_plan_to_register` /
`emit_prepared_home_publication_plan_to_register` helpers together. Keep
`emit_value_publication_to_register` in place and include the new owner header
from current direct clients.

## Watchouts

- Keep this route focused on value materialization authority.
- Do not fold in edge-copy cleanup, publication cleanup, calls cleanup, or
  broad AArch64 pipeline redesign.
- Treat expectation weakening, unsupported downgrades, text-emission contracts,
  and named-test shortcuts as route failures.
- Leave unrelated transient `review/` artifacts untouched.
- `emit_value_publication_to_register` is the fan-out point and should not be
  the first move; it recursively consumes most leaf materializers and is used by
  calls, stores, edge copies, publication, and branch fusion hooks.
- `dispatch.cpp` owns route selection for the seven `lower_*`/`make_*`
  instruction hooks; moving those before the leaf helpers risks a broad
  dispatch-route churn packet.
- The clang header query returned declarations but reported a missing
  `ast.hpp` include under the ad hoc flags route; the compile-database
  `dispatch_value_materialization.cpp` symbol/caller/callee checks were clean.

## Proof

No build or test proof required by this audit-only packet. Read-only proof
commands used: `c4c-clang-tool function-signatures` on
`dispatch_value_materialization.hpp`, `c4c-clang-tool-ccdb list-symbols` plus
`function-callers`/`function-callees` on
`dispatch_value_materialization.cpp`, targeted `function-callers` checks on
`dispatch.cpp`, `memory_store_sources.cpp`, and `dispatch_edge_copies.cpp`,
`rg` include/call searches, and `ctest --test-dir build -N` to identify focused
proof subset candidates. No `test_after.log` was produced because the delegated
proof explicitly required no build or test run.
