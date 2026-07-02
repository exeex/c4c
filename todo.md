Status: Active
Source Idea Path: ideas/open/540_rv64_object_select_edge_publication_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract Predecessor Edge Fragment Helpers

# Current Packet

## Just Finished

Completed Step 3 of `plan.md`: extracted predecessor-edge select publication
bundle admission predicates from `object_emission.cpp` into
`prepared_edge_publication_emit.cpp` / `.hpp` without changing object-side
traversal or fragment emission.

Moved helper ownership:

- `prepared_predecessor_select_publication_bundle_is_stack_join_materialized`
- `prepared_predecessor_select_publication_bundle_is_rv64_object_admitted`
- local prepared value-id lookup needed by those predicates

Boundary notes:

- `prepared_edge_publication_emit.hpp` now publishes
  `PreparedSelectPublicationStackHomePredicate` so the prepared-side
  predecessor admission walk can call the one retained object-side stack-home
  exception without taking over its internals.
- `object_emission.cpp` now passes
  `prepared_select_publication_destination_is_stack_home` into the prepared
  predecessor admission APIs.
- `fragment_for_predecessor_select_publication_pointer_stack_source_to_gpr` and
  `fragment_for_predecessor_select_publication_gpr_to_stack_destination` remain
  object-side because they still call object encoder utilities
  (`rv64_register_number`, `append_rv64_load_stack_to_register`,
  `append_rv64_store_register_to_stack`).
- `rv64_select_publication_bundle_rejection_diagnostic` remains object-side
  because it is diagnostic-heavy and traversal-aware.

## Suggested Next

Supervisor should delegate Step 4: inspect and narrow select-source producer
dependencies, moving only helpers whose ownership is naturally
edge-publication or scalar and whose context can be expressed through narrow
parameters.

## Watchouts

- Preserve prepared publication facts, fallback behavior, diagnostics, object bytes, tests, expectations, unsupported markers, and runtime contracts.
- Do not hide `fragment_for_prepared_instruction` fanout behind another broad dispatcher.
- Do not turn scalar helpers into select/publication catch-all ownership.
- Predecessor fragment bodies still call object-side encoder utilities and
  should remain parked until an explicit narrow encoder boundary exists.
- `prepared_select_publication_destination_is_stack_home` remains object-side
  and should not be folded into the prepared publication API without first
  separating its value-home lookup dependency.
- Step 4 must avoid dragging whole-function traversal or
  `fragment_for_prepared_instruction` fanout into `prepared_edge_publication_emit.*`.
- Leave the existing untracked review artifact untouched.

## Proof

Delegated proof passed and wrote `test_after.log`:

```text
bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_prepared_edge_publication|dump_riscv64_.*select|codegen_route_riscv64_.*select|rv64_runtime_riscv64_pointer_typed_select_publication|rv64_runtime_riscv64_short_circuit_select_false_lhs)'" > test_after.log 2>&1
```

Result: build succeeded; selected CTest subset passed 16/16.
