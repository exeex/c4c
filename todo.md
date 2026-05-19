Status: Active
Source Idea Path: ideas/open/311_aarch64_selected_call_boundary_move_preparation_printing.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Machine-Printer Semantics

# Current Packet

## Just Finished

Step 3 added focused `backend_aarch64_machine_printer` coverage for the
repaired selected AArch64 call-boundary stack move shapes. The printer now has
positive selected-node coverage for the prepared frame-slot to outgoing stack
store and the prepared aggregate-address stack-copy assembler node, plus
shape-specific fail-closed coverage proving unselected stack argument shapes
and incomplete aggregate-copy assembler nodes are rejected by existing printer
gates.

## Suggested Next

Proceed to Step 4 focused proof and residual classification. Confirm the old
selected call-boundary printer diagnostic and the later aggregate stack-copy
residual remain gone, then record whether any remaining `00140.c` issue is
outside this selected call-boundary move preparation/printing owner.

## Watchouts

- Do not suppress diagnostics or bypass the selected-machine-node printer gate.
- Do not mark a call-boundary move selected without printable prepared source
  and destination facts.
- The old printer residual
  `call-boundary move node is outside the selected register call-boundary move subset`
  is gone from the delegated proof.
- The prior aggregate/byval stack-copy residual
  `call-boundary stack argument move requires AArch64 stack-copy lowering` is
  gone from the delegated proof.
- Step 3 coverage is intentionally printer-facing; it proves selected
  machine-node printing and fail-closed gates without changing call lowering,
  expectations, allowlists, or unsupported classifications.
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

Build succeeded. `backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`,
`backend_aarch64_instruction_dispatch`, and
`c_testsuite_aarch64_backend_src_00140_c` passed. Both the old generic
selected-register residual and the narrower aggregate stack-copy residual are
gone from the delegated proof. `test_after.log` contains the preserved proof
output.
