Status: Active
Source Idea Path: ideas/open/311_aarch64_selected_call_boundary_move_preparation_printing.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish AArch64 Stack Call-Argument Destination Offsets

# Current Packet

## Just Finished

Step 2 continued the AArch64 stack call-argument handoff in
`src/backend/mir/aarch64/codegen/calls.cpp`. Prepared aggregate-address
source registers for byval/stack call arguments now lower to a selected
aggregate stack-copy machine node that carries structured source pointer
memory and destination outgoing-stack memory operands. The copy expands
semantically by prepared size into supported AArch64 load/store chunks instead
of admitting an unselected `CallBoundaryMoveInstructionRecord`.

## Suggested Next

Proceed to Step 3 printer semantics or supervisor review. The focused
aggregate/byval stack-copy residual for `00140.c` is gone; keep any next
packet scoped to selected machine-printer semantics for the repaired
call-boundary shapes unless a new exact residual appears.

## Watchouts

- Do not suppress the diagnostic or bypass the selected-machine-node printer
  gate.
- Do not mark a call-boundary move selected without printable prepared source
  and destination facts.
- The old printer residual
  `call-boundary move node is outside the selected register call-boundary move subset`
  is gone from the delegated proof.
- The prior aggregate/byval stack-copy residual
  `call-boundary stack argument move requires AArch64 stack-copy lowering` is
  gone from the delegated proof.
- The scalar frame-slot to outgoing stack-slot path intentionally supports only
  1, 4, and 8 byte stack slots because those are the currently printable
  scratch load/store widths in the AArch64 memory printer.
- Aggregate-address stack copies currently expand into 8, 4, and 1 byte
  chunks from the prepared aggregate size, using the prepared pointer register
  and destination stack offset as the semantic authority.
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
`backend_aarch64_instruction_dispatch` passed. The focused C probe
`c_testsuite_aarch64_backend_src_00140_c` also passed. Both the old generic
selected-register residual and the narrower aggregate stack-copy residual are
gone from the delegated proof. `test_after.log` contains the preserved proof
output.

The supervisor also ran:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

The broader backend bucket passed: 139/139 tests.
