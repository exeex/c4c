Status: Active
Source Idea Path: ideas/open/540_rv64_object_select_edge_publication_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Narrow Select Publication Move Helpers

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: extracted the narrow select publication move
admission/rejection helpers from `object_emission.cpp` into
`prepared_edge_publication_emit.cpp` / `.hpp` without behavior changes.

Moved helper ownership:

- `prepared_select_publication_move_is_rv64_object_admitted`
- `prepared_select_publication_pointer_stack_source_to_gpr_is_admitted`
- `prepared_select_publication_gpr_to_stack_destination_is_admitted`
- `edge_publication_move_intent_status_name`
- `prepared_edge_publication_lookup_status_name`
- `rv64_select_publication_move_rejection_reason`
- `prepared_select_publication_pointer_stack_source_to_gpr_matches_bundle`
- `prepared_select_publication_gpr_to_stack_destination_matches_bundle`

Boundary notes:

- `prepared_edge_publication_emit.hpp` now publishes only the predicates,
  status-name helpers, rejection reason helper, and bundle-match helpers still
  used by object-side orchestration/diagnostics.
- The moved code uses existing prepared frame/register helpers for register and
  immediate validation, and a local edge-publication scalar-size classifier for
  the narrow stack-destination admission check.
- `object_emission.cpp` now includes `prepared_edge_publication_emit.hpp` and
  no longer owns those narrow helper bodies.
- `prepared_select_publication_destination_is_stack_home` remains object-side
  because it still reaches through `PreparedNameTables`,
  `PreparedFunctionLookups`, `PreparedParallelCopyMove`,
  `prepared_value_id_for_named_value`, `consume_edge_publication_move_intent`,
  and `prepared_value_home_for_id`.

## Suggested Next

Supervisor should delegate Step 3: decide whether to move the predecessor-edge
admission predicates behind `prepared_edge_publication_emit.*` while keeping
predecessor fragment bodies object-side until their encoder dependencies are
made explicit.

## Watchouts

- Preserve prepared publication facts, fallback behavior, diagnostics, object bytes, tests, expectations, unsupported markers, and runtime contracts.
- Do not hide `fragment_for_prepared_instruction` fanout behind another broad dispatcher.
- Do not turn scalar helpers into select/publication catch-all ownership.
- Predecessor fragment bodies still call object-side encoder utilities
  (`rv64_register_number`, `append_rv64_load_stack_to_register`,
  `append_rv64_store_register_to_stack`) and should remain parked unless the
  next packet publishes a narrow helper boundary for those dependencies.
- `rv64_select_publication_bundle_rejection_diagnostic` is diagnostic-heavy and
  traversal-aware; it remains object-side for now.
- Leave the existing untracked review artifact untouched.

## Proof

Delegated proof passed and wrote `test_after.log`:

```text
bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_prepared_edge_publication|dump_riscv64_.*select|codegen_route_riscv64_.*select|rv64_runtime_riscv64_pointer_typed_select_publication|rv64_runtime_riscv64_short_circuit_select_false_lhs)'" > test_after.log 2>&1
```

Result: build succeeded; selected CTest subset passed 16/16.
