Status: Active
Source Idea Path: ideas/open/311_aarch64_selected_call_boundary_move_preparation_printing.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish AArch64 Stack Call-Argument Destination Offsets

# Current Packet

## Just Finished

Step 2 published AArch64 stack call-argument destination offsets through
`src/backend/prealloc/regalloc/call_return_abi.cpp` by allowing
`call_arg_destination_stack_offset_bytes` to compute offsets for AArch64
stack-slot call arguments. Focused AArch64 MIR coverage now prepares a semantic
byval stack call argument and proves the prepared call argument plan, the
before-call move, and the ABI binding all carry
`destination_stack_offset_bytes=0`.

## Suggested Next

Execute the next Step 2 follow-on packet in the AArch64 codegen handoff:
teach `lower_before_call_move` and the selected-machine-node printer path to
handle or fail closed with a more specific diagnostic for prepared stack-slot
call-argument moves whose source and destination stack offset facts are now
present.

## Watchouts

- Do not suppress the diagnostic or bypass the selected-machine-node printer
  gate.
- Do not mark a call-boundary move selected without printable prepared source
  and destination facts.
- The missing AArch64 destination stack offset publication is no longer the
  current blocker. The remaining `00140.c` failure is that a stack-slot
  call-argument move reaches the AArch64 machine printer as an unselected
  `CallBoundaryMoveInstructionRecord`.
- `lower_before_call_move` currently only selects register call-boundary moves;
  stack-slot call-argument moves still need an AArch64 lowering contract or a
  narrower fail-closed gate before printing.
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
`backend_aarch64_instruction_dispatch` passed, including new focused coverage
for AArch64 stack call-argument `destination_stack_offset_bytes` publication.
The focused proof still failed at `c_testsuite_aarch64_backend_src_00140_c`
with the next stack-slot call-boundary lowering/printing residual:
`DeferredUnsupported: call-boundary move node is outside the selected register
call-boundary move subset`.

The supervisor then ran a matching broader backend guard through the dirty-tree
stash flow:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_before.log 2>&1
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1
```

Both canonical logs report 139/139 backend tests passed with no new failures.
