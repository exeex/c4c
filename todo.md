Status: Active
Source Idea Path: ideas/open/311_aarch64_selected_call_boundary_move_preparation_printing.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish AArch64 Stack Call-Argument Destination Offsets

# Current Packet

## Just Finished

Step 2 continued the AArch64 stack call-argument handoff in
`src/backend/mir/aarch64/codegen/calls.cpp`. Prepared frame-slot source to
stack-slot destination call-argument moves now lower as selected memory stores
when the source copy width is a printer-supported 1, 4, or 8 byte scalar
stack slot. Register aggregate-address to outgoing stack-slot call-argument
moves now remain gated as unselected `CallBoundaryMoveInstructionRecord`s with
a narrow stack-copy diagnostic instead of the old generic register-subset
diagnostic.

## Suggested Next

Decide whether idea 311 should proceed to a semantic aggregate-address stack
copy lowering packet or move to Step 3 printer semantics. The remaining
semantic gap is real aggregate/byval memory copy lowering from a prepared
aggregate address register to the outgoing call stack area; it is no longer the
old generic selected-printer admission failure.

## Watchouts

- Do not suppress the diagnostic or bypass the selected-machine-node printer
  gate.
- Do not mark a call-boundary move selected without printable prepared source
  and destination facts.
- The old printer residual
  `call-boundary move node is outside the selected register call-boundary move subset`
  is gone from the delegated proof.
- The current residual for aggregate-address stack copies is:
  `call-boundary stack argument move requires AArch64 stack-copy lowering`.
- The scalar frame-slot to outgoing stack-slot path intentionally supports only
  1, 4, and 8 byte stack slots because those are the currently printable
  scratch load/store widths in the AArch64 memory printer.
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
`c_testsuite_aarch64_backend_src_00140_c` failed with the distinct residual:
`DeferredUnsupported: call-boundary stack argument move requires AArch64
stack-copy lowering`. The old generic selected register call-boundary move
diagnostic is gone. `test_after.log` contains the preserved proof output.

The supervisor also ran:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

The broader backend bucket passed: 139/139 tests.
