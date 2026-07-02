Status: Active
Source Idea Path: ideas/open/540_rv64_object_select_edge_publication_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Select And Publication Ownership

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: mapped select-source, publication move,
predecessor-edge, and rejection-diagnostic helper ownership without
implementation edits.

Helper groups and proposed ownership:

- Already edge-publication-owned API: `consume_edge_publication_move_intent`
  overloads and `append_edge_publication_move_instruction` live behind
  `prepared_edge_publication_emit.cpp` / public declarations in `emit.hpp`.
  Their adapter already owns prepared publication lookup, route3/route5
  agreement, stack-source/register-source rendering, and emitted move text.
- First extraction target: move the narrow select publication move admission
  and diagnostic helpers from `object_emission.cpp` to
  `prepared_edge_publication_emit.cpp`, published through
  `prepared_edge_publication_emit.hpp` only where object orchestration still
  needs calls. This group is `prepared_select_publication_move_is_rv64_object_admitted`,
  `prepared_select_publication_pointer_stack_source_to_gpr_is_admitted`,
  `prepared_select_publication_gpr_to_stack_destination_is_admitted`,
  `edge_publication_move_intent_status_name`,
  `prepared_edge_publication_lookup_status_name`,
  `rv64_select_publication_move_rejection_reason`,
  `prepared_select_publication_pointer_stack_source_to_gpr_matches_bundle`,
  and `prepared_select_publication_gpr_to_stack_destination_matches_bundle`.
  These helpers depend mostly on `EdgePublicationMoveIntent`, prepared
  publication/bundle facts, register names, scalar size, and 12-bit immediate
  checks, so they are edge-publication policy rather than whole-object
  traversal.
- First-target companion to keep object-side unless the Step 2 API is made
  explicit: `prepared_select_publication_destination_is_stack_home` also
  classifies select publication admission, but it currently reaches through
  `PreparedNameTables`, `PreparedFunctionLookups`, `PreparedParallelCopyBundle`,
  `PreparedParallelCopyMove`, `prepared_value_id_for_named_value`,
  `consume_edge_publication_move_intent`, and `prepared_value_home_for_id`.
  It can move only if Step 2 exposes the value-id/home lookup dependency
  cleanly, otherwise leave it object-side for this packet.
- Rejection diagnostic group: `append_optional_value`,
  `append_optional_block_label`, `append_select_publication_intent_evidence`,
  and `rv64_select_publication_bundle_rejection_diagnostic` format the detailed
  select-publication failure string. They are good follow-on edge-publication
  candidates after the move-admission predicates move, but not the first body
  to extract because the diagnostic helper also formats traversal event,
  function/block labels, move-bundle metadata, and object-route prepared
  consumer context.
- Predecessor-edge admission/fragment group:
  `prepared_predecessor_select_publication_bundle_is_stack_join_materialized`,
  `prepared_predecessor_select_publication_bundle_is_rv64_object_admitted`,
  `fragment_for_predecessor_select_publication_pointer_stack_source_to_gpr`,
  and `fragment_for_predecessor_select_publication_gpr_to_stack_destination`.
  These belong conceptually with prepared edge publication because they are
  driven by `PreparedParallelCopyBundle` and consumed publication facts, but
  they should follow Step 2 because the fragment helpers still call object-side
  encoder utilities (`rv64_register_number`, `append_rv64_load_stack_to_register`,
  `append_rv64_store_register_to_stack`) and rely on the admission helpers.
- Select-source producer and carrier authority group:
  `rv64_select_edge_binary_operand_is_register_or_immediate`,
  `find_available_rv64_select_edge_cast_dependency_authority`,
  `rv64_select_edge_binary_operand_is_register_immediate_or_cast_authorized`,
  `rv64_select_edge_binary_has_available_cast_dependency_authority`,
  `rv64_select_edge_dependency_operand_current_source_register`,
  `prepared_cast_is_available_select_edge_dependency_authority_source`,
  `is_authorized_source_producer_operand`,
  `fragment_for_prepared_select_edge_binary_with_cast_dependencies`,
  `prepared_select_edge_binary_source_has_only_carrier_uses`,
  `prepared_select_edge_binary_source_has_carrier_alias_authority`,
  `prepared_select_edge_binary_source_has_authorized_consumers`,
  `prepared_select_edge_source_value_needs_binary_producer`,
  `fragment_for_prepared_select_edge_source_dependencies`, and
  `fragment_for_prepared_select_edge_source_producer`. Defer these until Step 4:
  they inspect whole BIR functions, dependency/carrier authority records, and
  call `fragment_for_prepared_binary`, so moving them first risks hiding broad
  instruction fanout behind a new dispatcher.
- Object-side orchestration that must remain in `object_emission.cpp`:
  `fragment_for_prepared_instruction`, `fragment_for_prepared_move_bundle`, the
  traversal loop in `prepared_function_to_object_function`, and the places that
  decide whether to emit a predecessor-edge fragment or fall back to generic
  move-bundle emission. These depend on whole-function traversal, compare
  tracking, block events, broad dispatch, and rejection construction.

Compile/header prerequisites for the first extraction:

- Add declarations in `prepared_edge_publication_emit.hpp` for only the helper
  predicates/reason functions still called from `object_emission.cpp`; keep
  internal-only status-name and evidence helpers in the `.cpp` anonymous
  namespace when possible.
- Ensure moved helpers can see `EdgePublicationMoveIntent`,
  `EdgePublicationMoveIntentStatus`, `PreparedParallelCopyBundle`,
  `PreparedJoinTransferCarrierKind`, `PreparedParallelCopyExecutionSite`,
  `PreparedParallelCopyStepKind`, `bir::TypeKind`, `BlockLabelId`, and the
  scalar/register/immediate helpers they require.
- Avoid introducing a dependency from `prepared_edge_publication_emit.cpp` back
  to `object_emission.cpp`; if `rv64_register_number`,
  `rv64_scalar_memory_size_for_type`, or stack load/store appenders are needed,
  use existing prepared scalar/frame helpers or publish a narrow shared helper
  before moving fragment bodies.
- Preserve the existing diagnostic strings and `select_publication_rejection_reason`
  tokens byte-for-byte during extraction.

## Suggested Next

Supervisor should delegate Step 2: extract only the narrow select publication
move admission/rejection helpers into `prepared_edge_publication_emit.cpp` /
`.hpp`, leaving predecessor fragments, select-source producer helpers, and broad
object traversal in `object_emission.cpp`.

## Watchouts

- Preserve prepared publication facts, fallback behavior, diagnostics, object bytes, tests, expectations, unsupported markers, and runtime contracts.
- Do not hide `fragment_for_prepared_instruction` fanout behind another broad dispatcher.
- Do not turn scalar helpers into select/publication catch-all ownership.
- The first extraction should not move predecessor fragment bodies until the
  admission helpers compile through the prepared edge publication header.
- `rv64_select_publication_bundle_rejection_diagnostic` is diagnostic-heavy and
  traversal-aware; moving it before the predicates would create a larger API
  surface than the first extraction needs.
- Leave the existing untracked review artifact untouched.

## Proof

No build required for this mapping-only Step 1 packet, and no `test_after.log`
was written.

Exact Step 2/select-publication proof command recorded for the first extraction:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_prepared_edge_publication|dump_riscv64_.*select|codegen_route_riscv64_.*select|rv64_runtime_riscv64_pointer_typed_select_publication|rv64_runtime_riscv64_short_circuit_select_false_lhs)'
```
