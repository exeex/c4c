Status: Active
Source Idea Path: ideas/open/670_byval_frame_slot_object_runtime_binaryinst.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Byval Object-Runtime Regression Safety

# Current Packet

## Just Finished

Completed Step 3 of `plan.md`: proved byval object-runtime regression safety
for the focused object-runtime row plus nearby byval route/runtime rows.

The delegated five-row proof passed. Step 2 had no current repair owner because
Step 1 already showed the former `BinaryInst` object-runtime boundary is stale:
the focused object-runtime target now emits, links, and runs successfully, with
prepared byval/frame-slot facts present and no fresh diagnostic exposing a
repair layer.

## Suggested Next

Run Step 4 close readiness for idea 670 using the supervisor-selected closure
scope.

## Watchouts

- Treat byval route/runtime rows from idea 659 as regression surfaces, not the
  implementation target.
- Do not reopen prepared byval call-boundary behavior, generic RISC-V
  object-emission work, or object-runtime `BinaryInst` lowering without fresh
  failing evidence from the target row.
- Do not use testcase names, final object bytes, fixed registers, or source
  shape as the object-runtime instruction authority.
- The old `BinaryInst` diagnostic is stale evidence only; do not patch against
  it unless a fresh reproduction reappears.

## Proof

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload|backend_codegen_route_riscv64_byval_aggregate_fixed_call|backend_codegen_route_riscv64_byval_preserved_pointer_args|backend_rv64_runtime_riscv64_byval_aggregate_fixed_call|backend_rv64_runtime_riscv64_byval_preserved_pointer_args)$') > test_after.log 2>&1
```

Result: exit `0`; build was up to date and all 5 selected tests passed:
`backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload`,
`backend_codegen_route_riscv64_byval_aggregate_fixed_call`,
`backend_codegen_route_riscv64_byval_preserved_pointer_args`,
`backend_rv64_runtime_riscv64_byval_aggregate_fixed_call`, and
`backend_rv64_runtime_riscv64_byval_preserved_pointer_args`.
Proof log: `test_after.log`.
