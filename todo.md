Status: Active
Source Idea Path: ideas/open/311_aarch64_selected_call_boundary_move_preparation_printing.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Selected Call-Boundary Move Admission

# Current Packet

## Just Finished

Lifecycle switched from umbrella idea 295 to focused idea 311. No compiler
implementation was performed during the split.

## Suggested Next

Execute Step 1 from `plan.md`: localize why the `00140.c` call-boundary move
record reaches the AArch64 machine printer as `DeferredUnsupported`, then
record the missing prepared source/destination fact or the exact prerequisite
owner if the failure is outside this scope.

Suggested proof command after implementation:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00140_c)$' > test_after.log 2>&1
```

## Watchouts

- Do not suppress the diagnostic or bypass the selected-machine-node printer
  gate.
- Do not mark a call-boundary move selected without printable prepared source
  and destination facts.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, proof-log policy, or CTest registration.
- Reject filename-only, `struct foo`-only, argument-index-only, source-shape,
  or emitted-mnemonic shortcuts.
- Keep direct-call shuffle, direct vararg, address-of-local, runtime,
  timeout/output-storm, other machine-printer, and semantic `lir_to_bir`
  residual buckets parked under idea 295 unless separate evidence justifies a
  new split.

## Proof

No build or CTest was run by the plan owner. Lifecycle-only split based on the
supervisor-provided fresh `test_after.log` failure evidence for
`c_testsuite_aarch64_backend_src_00140_c`.
