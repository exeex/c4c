Status: Active
Source Idea Path: ideas/open/311_aarch64_selected_call_boundary_move_preparation_printing.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Selected Call-Boundary Move Admission

# Current Packet

## Just Finished

Step 1 localized the `00140.c` failure to the first byval aggregate call
argument in `main`: prepared BIR publishes a before-call move with
`destination_kind=call_argument_abi`, `destination_storage=stack_slot`,
`abi_index=0`, and `reason=call_arg_register_to_stack`, but neither the move
nor its ABI binding carries `destination_stack_offset_bytes` for AArch64. The
record is then admitted by `lower_before_call_move` as a
`CallBoundaryMoveInstructionRecord`; `call_boundary_move_selection_status`
correctly leaves it `DeferredUnsupported` because it is outside the selected
register call-boundary move subset, and the machine printer rejects it at
function 0 block 0 instruction 3.

## Suggested Next

Decide whether this focused route should first grow AArch64 stack call-argument
destination support or split that prerequisite to the prealloc call ABI owner.
The first implementation target is
`src/backend/prealloc/regalloc/call_return_abi.cpp`:
`call_arg_destination_stack_offset_bytes` is currently x86_64-only, so AArch64
byval/stack call arguments cannot publish the prepared destination stack offset
needed before AArch64 codegen can lower or reject the stack-slot call-boundary
move semantically.

## Watchouts

- Do not suppress the diagnostic or bypass the selected-machine-node printer
  gate.
- Do not mark a call-boundary move selected without printable prepared source
  and destination facts.
- The missing destination fact is a stack argument offset for AArch64, not a
  missing register source: `main` already has the source value `%lv.f` in x20,
  while the byval aggregate-address argument has `dest_bank=none` and no stack
  offset in the prepared call plan.
- `lower_before_call_move` currently only guards incomplete selected register
  argument moves; stack-slot call-argument moves can still reach the printer as
  unselected `CallBoundaryMoveInstructionRecord`s.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, proof-log policy, or CTest registration.
- Reject filename-only, `struct foo`-only, argument-index-only, source-shape,
  or emitted-mnemonic shortcuts.
- Keep direct-call shuffle, direct vararg, address-of-local, runtime,
  timeout/output-storm, other machine-printer, and semantic `lir_to_bir`
  residual buckets parked under idea 295 unless separate evidence justifies a
  new split.

## Proof

Ran the delegated proof command:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00140_c)$' > test_after.log 2>&1
```

`backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, and
`backend_aarch64_instruction_dispatch` passed. The proof still fails at
`c_testsuite_aarch64_backend_src_00140_c` with the localized printer rejection:
`DeferredUnsupported: call-boundary move node is outside the selected register
call-boundary move subset`. Proof log: `test_after.log`.
