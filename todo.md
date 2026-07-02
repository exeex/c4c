Status: Active
Source Idea Path: ideas/open/540_rv64_object_select_edge_publication_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Narrow Select Source Producer Dependencies

# Current Packet

## Just Finished

Completed Step 4 of `plan.md`: narrowed select-source producer helper
ownership by moving the carrier-alias authority predicates from
`object_emission.cpp` into `prepared_edge_publication_emit.cpp` / `.hpp`.

Moved helper ownership:

- `prepared_select_edge_binary_source_has_carrier_alias_authority`
- `prepared_select_is_authorized_carrier_alias`

Boundary notes:

- The moved helpers are record-level predicates over prepared carrier-alias
  authority/publication records or one explicit `SelectInst`; they do not take
  over whole-function traversal, fragment emission, or dispatch.
- `prepared_select_edge_binary_source_has_only_carrier_uses` remains
  object-side because it scans the whole BIR function for use closure.
- `prepared_select_edge_binary_source_has_authorized_consumers` remains
  object-side because it combines the whole-function use scan with the moved
  record-level carrier-alias predicate.
- `fragment_for_prepared_select_edge_source_dependencies` and
  `fragment_for_prepared_select_edge_source_producer` remain object-side
  because they walk function blocks, call `fragment_for_prepared_binary`, and
  drive object fragment assembly.
- Cast-dependency authority helpers remain object-side because they either
  scan whole-function uses or feed object-side materialization fragments.

## Suggested Next

Supervisor should delegate the next Step 4/5 packet to review whether any
diagnostic-only select publication helpers can move without pulling in object
encoder utilities, or proceed to the runbook review step if no narrower
extraction remains.

## Watchouts

- Preserve prepared publication facts, fallback behavior, diagnostics, object bytes, tests, expectations, unsupported markers, and runtime contracts.
- Do not hide `fragment_for_prepared_instruction` fanout behind another broad dispatcher.
- Do not turn scalar helpers into select/publication catch-all ownership.
- Predecessor fragment bodies still call object-side encoder utilities and
  should remain parked until an explicit narrow encoder boundary exists.
- Select-source producer fragment bodies still depend on BIR block traversal,
  scalar fragment emission, cast dependency materialization, and object
  fragment assembly; keep them parked unless a separate narrow boundary is
  designed.
- `prepared_select_publication_destination_is_stack_home` remains object-side
  and should not be folded into the prepared publication API without first
  separating its value-home lookup dependency.
- Leave the existing untracked review artifact untouched.

## Proof

Delegated proof passed and wrote `test_after.log`:

```text
bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_prepared_edge_publication|dump_riscv64_.*select|codegen_route_riscv64_.*select|rv64_runtime_riscv64_pointer_typed_select_publication|rv64_runtime_riscv64_short_circuit_select_false_lhs)'" > test_after.log 2>&1
```

Result: build succeeded; selected CTest subset passed 16/16.
