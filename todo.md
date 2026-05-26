Status: Active
Source Idea Path: ideas/open/11_aarch64_calls_file_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Remaining Calls File Roles

# Current Packet

## Just Finished

Step 1 - Audit Remaining Calls File Roles completed as an audit-only packet.
No implementation files, tests, build metadata, plan files, ideas, review files,
or test logs were changed.

Inspected files and symbols:

- `src/backend/mir/aarch64/codegen/calls.hpp`: public declaration inventory via
  `c4c-clang-tool function-signatures`; header parse reported a missing
  `mir/printer.hpp` include on the flags route after producing declarations,
  so declaration use was cross-checked with `rg`.
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.hpp`: public bridge
  declaration inventory via `c4c-clang-tool function-signatures`; same flags
  route include warning after producing declarations, cross-checked with `rg`.
- `src/backend/mir/aarch64/codegen/calls.cpp`: `find_prepared_call_plan`,
  `require_prepared_call_plan`, `make_call_boundary_move_instruction`,
  `make_call_boundary_abi_binding_instruction`, `make_call_instruction`,
  `lower_prepared_call_instruction`.
- `src/backend/mir/aarch64/codegen/calls_common.cpp`: `align_to`,
  `incoming_stack_argument_size_bytes`,
  `incoming_stack_argument_alignment_bytes`,
  `outgoing_stack_argument_bytes`,
  `outgoing_stack_argument_base_register`,
  `entry_param_uses_incoming_stack`, `named_incoming_stack_bytes`,
  `function_has_call`, `fixed_frame_adjustment_bytes`,
  `va_start_overflow_area_stack_offset`, `register_class_from_bank`.
- `src/backend/mir/aarch64/codegen/calls_operand_adapters.cpp`:
  `complete_full_width_f128_carrier`, `complete_f128_constant_carrier`,
  `find_prepared_f128_carrier_in_module`,
  `scalar_fp_view_from_register_name`, `scalar_view_from_register_name`,
  `scalar_size_from_register_view`, `register_name_with_expected_view`,
  `make_register_operand_from_prepared_authority`,
  `make_f128_q_register_operand_from_carrier`,
  `make_scalar_call_argument_immediate`, `scalar_integer_register_view`,
  `scalar_integer_register_view_from_size`,
  `scalar_integer_type_from_size`.
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`:
  `is_aarch64_byval_register_lane_move`,
  `collect_byval_register_lane_stores`,
  `make_byval_register_lane_prepared_source`,
  `aggregate_lane_store_memory`.
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`:
  `prepared_indirect_byval_extent_bytes`,
  `make_selected_call_argument_source`, `find_move_bundle`,
  `call_boundary_move_reloads_materialized_address`,
  `order_before_call_moves_for_source_preservation`,
  `lower_before_call_moves`, `lower_after_call_moves`,
  `lower_before_return_moves`, `lower_value_moves`, plus private move,
  publication, preservation, byval, immediate, stack, and register helpers.
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`:
  `lower_scalar_call_argument_producers`, `lower_call_instruction`,
  `materialize_call_boundary_source_to_destination`,
  `retarget_call_boundary_source_to_emitted_scalar`,
  `record_call_boundary_destination`,
  `record_call_boundary_source_in_destination`,
  `call_boundary_move_reloads_prepared_stack_source`,
  `source_register_conflicts_with_materialized_address`,
  `materialize_indirect_call_callee_to_prepared_register`,
  `record_call_result_source_register`,
  `retarget_fpr_call_result_store_value_to_emitted_scalar`,
  `materialize_missing_frame_slot_call_arguments`,
  `publish_stack_preserved_call_values`.

Current build entries:

- `src/backend/CMakeLists.txt` lists these AArch64 calls translation units:
  `calls.cpp`, `calls_byval_aggregates.cpp`, `calls_common.cpp`,
  `calls_dispatch_bridge.cpp`, `calls_moves.cpp`,
  `calls_operand_adapters.cpp`.
- `build/compile_commands.json` has matching object entries for all six:
  `calls.cpp.o`, `calls_byval_aggregates.cpp.o`, `calls_common.cpp.o`,
  `calls_dispatch_bridge.cpp.o`, `calls_moves.cpp.o`,
  `calls_operand_adapters.cpp.o`.

Active clients:

- `dispatch.cpp` includes both `calls.hpp` and `calls_dispatch_bridge.hpp` and
  directly calls `find_prepared_call_plan`, `lower_value_moves`,
  `lower_before_call_moves`, `order_before_call_moves_for_source_preservation`,
  `call_boundary_move_reloads_materialized_address`,
  `lower_call_instruction`, `lower_after_call_moves`,
  `lower_before_return_moves`, and bridge helpers. AST caller checks confirmed
  `dispatch_prepared_block` as the direct caller for `lower_call_instruction`,
  `lower_before_call_moves`, `lower_after_call_moves`, `lower_value_moves`, and
  `lower_before_return_moves`.
- `calls_dispatch_bridge.cpp` calls `require_prepared_call_plan`,
  `lower_prepared_call_instruction`, `find_move_bundle`,
  `make_selected_call_argument_source`, and
  `scalar_integer_register_view_from_size`.
- `calls_moves.cpp` calls the byval aggregate helpers, operand adapters, common
  stack helpers, and instruction factories.
- `globals.cpp`, `f128.cpp`, and `i128_ops.cpp` include `calls.hpp`; current
  `rg` hits show `globals.cpp` actively uses `find_prepared_call_plan`.
- Tests that directly include the calls headers are
  `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`,
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`, and
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`.

Role classification:

- `calls.cpp`: emission-only target call instruction owner with prepared-plan
  lookup and diagnostics. Keep separate for now; it is the natural owner for
  `lower_prepared_call_instruction` and instruction-record construction.
- `calls_common.cpp`: shared helper. It owns stack sizing/alignment, incoming
  stack argument math, fixed frame adjustment, variadic overflow offset, and
  register-bank-to-class conversion. Safe only for helper consolidation, not
  semantic lowering.
- `calls_operand_adapters.cpp`: target adapter/shared helper. It converts
  prepared register, view, immediate, and F128 carrier facts into AArch64
  operands. It does not choose semantic call-argument sources.
- `calls_byval_aggregates.cpp`: shared helper for byval aggregate lane memory
  reconstruction. It is currently only consumed by `calls_moves.cpp`; it is a
  possible later same-owner merge into `calls_moves.cpp`, but it is not the
  smallest Step 2 target because it is closer to semantic byval lowering than
  pure emission.
- `calls_moves.cpp`: semantic lowering. It owns call-boundary move lowering,
  value moves, preservation/republication materialization, source selection
  translation, byval use sites, immediate/cast materialization, and move bundle
  lookup. Do not merge broadly under this consolidation idea.
- `calls_dispatch_bridge.hpp` / `calls_dispatch_bridge.cpp`: dispatch bridge.
  It coordinates `dispatch.cpp` scalar state with call lowering, indirect
  callee materialization, missing frame-slot argument materialization, and
  preservation publication. Do not merge under this idea; dispatch cleanup
  belongs to idea `12`.

Safe small merge/deletion candidates:

- Best Step 2 candidate: merge `calls_operand_adapters.cpp` into
  `calls_common.cpp` as shared target-helper implementation, then remove
  `calls_operand_adapters.cpp` from `src/backend/CMakeLists.txt`. This is a
  thin target-adapter file, currently 307 lines, with no separate header and
  all declarations already exposed through `calls.hpp`.
- Secondary candidate after the adapter merge: move
  `calls_byval_aggregates.cpp` into `calls_moves.cpp` only if the supervisor
  accepts that it is same-owner helper consolidation for the existing move
  lowering owner. It should not be the first Step 2 packet because it carries
  byval aggregate source semantics.
- `calls_common.cpp` should not be deleted in the first consolidation packet;
  it is the lowest-risk recipient for shared helper code and remains needed by
  multiple call lowering files.

Files that must not be merged under this idea:

- Do not merge `calls_dispatch_bridge.cpp` or
  `calls_dispatch_bridge.hpp`; they are explicit dispatch bridge surfaces.
- Do not merge `calls_moves.cpp` wholesale with `calls.cpp`; that would mix
  semantic move lowering with call instruction emission.
- Do not merge `dispatch.cpp` participation into calls files; dispatch
  sequencing and scalar-state cleanup are out of scope.
- Do not rewrite ABI classification, prepared call-plan ownership, variadic
  helper semantics, preservation source selection, or expectation files as part
  of any consolidation packet.

`calls.hpp` declaration actions:

- Keep public for now: `find_prepared_call_plan`,
  `require_prepared_call_plan`, `lower_prepared_call_instruction`,
  `lower_before_call_moves`, `lower_after_call_moves`,
  `lower_before_return_moves`, `lower_value_moves`,
  `call_boundary_move_reloads_materialized_address`,
  `order_before_call_moves_for_source_preservation`, `find_move_bundle`, and
  `make_selected_call_argument_source`; these are active cross-file contracts
  between `dispatch.cpp`, `calls_dispatch_bridge.cpp`, and `calls_moves.cpp`.
- Keep public for now but consider hiding after the adapter/common merge:
  `complete_full_width_f128_carrier`, `complete_f128_constant_carrier`,
  `find_prepared_f128_carrier_in_module`,
  `scalar_fp_view_from_register_name`, `scalar_view_from_register_name`,
  `scalar_size_from_register_view`, `register_name_with_expected_view`,
  `make_register_operand_from_prepared_authority`,
  `make_f128_q_register_operand_from_carrier`,
  `make_scalar_call_argument_immediate`, `scalar_integer_register_view`,
  `scalar_integer_register_view_from_size`,
  `scalar_integer_type_from_size`. These should remain declared while both
  `calls_moves.cpp` and `calls_dispatch_bridge.cpp` call them.
- Move/hide only with the byval owner if that later packet runs:
  `AggregateRegisterLaneStore`, `is_aarch64_byval_register_lane_move`,
  `collect_byval_register_lane_stores`,
  `make_byval_register_lane_prepared_source`,
  `aggregate_lane_store_memory`, and
  `prepared_indirect_byval_extent_bytes`. These are effectively
  `calls_moves.cpp` helpers.
- Keep or move separately outside this packet: `append_call_diagnostic` appears
  declared in `calls.hpp` but the current callee query resolves bridge use to
  `dispatch_diagnostics.hpp`; do not change it during the first consolidation
  packet without a focused declaration cleanup check.
- Do not broaden `calls.hpp`; simplification should happen after source-file
  consolidation and compile proof.

Proof map for later packets:

- Adapter merge proof: build `c4c_backend`, then run focused tests
  `backend_aarch64_instruction_dispatch_test`,
  `backend_aarch64_call_boundary_owner_test`, and
  `backend_prepare_frame_stack_call_contract_test`. Also run a search proof
  that `calls_operand_adapters.cpp` no longer appears in
  `src/backend/CMakeLists.txt` or `build/compile_commands.json` after
  regeneration.
- Byval helper merge proof, if selected later: build `c4c_backend`, then run
  focused call-boundary/byval tests and any backend tests whose names include
  `aarch64`, `call`, `byval`, `aggregate`, `frame_stack`, or `prepare_frame`.
  Keep this separate from the adapter packet.
- Header simplification proof: build `c4c_backend` and rerun the direct header
  include tests listed above; use `rg` to confirm removed declarations have no
  remaining external references.

## Suggested Next

Delegate Step 2 as a bounded adapter consolidation packet: merge
`src/backend/mir/aarch64/codegen/calls_operand_adapters.cpp` into
`src/backend/mir/aarch64/codegen/calls_common.cpp`, remove the retired source
entry from `src/backend/CMakeLists.txt`, regenerate/verify build metadata, and
leave `calls.hpp` declarations intact unless compile proof shows a private
helper can be hidden without touching `dispatch.cpp` or `calls_moves.cpp`.

## Watchouts

- This idea is behavior-preserving consolidation only.
- Do not move new semantic authority, alter dispatch behavior, rewrite ABI
  classification, or weaken tests.
- Leave unrelated transient `review/` files untouched.
- `calls_dispatch_bridge.*` is a dispatch bridge and should not be merged here;
  idea `12` owns dispatch cleanup.
- Avoid making `calls_moves.cpp` larger in the first Step 2 packet. It is
  already the 4,521-line semantic lowering owner, so the lower-risk first
  deletion is the 307-line target-adapter source.
- `calls_byval_aggregates.cpp` looks same-owner with `calls_moves.cpp`, but it
  should remain separate until a focused byval packet because its helpers encode
  aggregate lane source semantics.
- `c4c-clang-tool` header parsing returned declarations before a missing
  `mir/printer.hpp` include error on the flags route; use compile-db queries on
  `.cpp` files plus `rg` when checking declaration deletion.

## Proof

Audit-only packet; no build required by supervisor.

Commands used:

- `c4c-clang-tool-ccdb list-symbols` on each remaining AArch64 calls
  translation unit.
- `c4c-clang-tool-ccdb function-signatures` on `calls_moves.cpp` and
  `calls_dispatch_bridge.cpp`.
- `c4c-clang-tool function-signatures` on `calls.hpp` and
  `calls_dispatch_bridge.hpp`.
- `c4c-clang-tool-ccdb function-callers` in `dispatch.cpp` for
  `lower_call_instruction`, `lower_before_call_moves`,
  `lower_after_call_moves`, `lower_value_moves`, and
  `lower_before_return_moves`.
- `c4c-clang-tool-ccdb function-callees` on
  `calls_dispatch_bridge.cpp::lower_call_instruction`.
- `rg` checks for current build entries, includes, public declarations, and
  active clients.

No `test_after.log` was produced because this packet was audit-only and the
delegated proof explicitly required no build.
