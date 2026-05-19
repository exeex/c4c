Status: Active
Source Idea Path: ideas/open/311_aarch64_selected_call_boundary_move_preparation_printing.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Run Focused Proof and Classify Residuals

# Current Packet

## Just Finished

Step 4 ran the focused proof and classified the `00140.c` result. The old
selected call-boundary diagnostic
`call-boundary move node is outside the selected register call-boundary move subset`
is gone. The later aggregate/byval stack-copy residual
`call-boundary stack argument move requires AArch64 stack-copy lowering` is
also gone. No remaining `c_testsuite_aarch64_backend_src_00140_c` residual is
present in the delegated focused proof.

## Suggested Next

Return to the supervisor for lifecycle handling of idea 311 now that the
focused Step 4 proof is green and `00140.c` has no residual in this subset.

## Watchouts

- The focused proof passed all four delegated tests, including
  `c_testsuite_aarch64_backend_src_00140_c`.
- No `00140.c` residual remains to classify in this focused proof.
- Keep any broader lifecycle decision with the supervisor or plan owner.
- Do not suppress diagnostics or bypass the selected-machine-node printer gate.
- Do not mark a call-boundary move selected without printable prepared source
  and destination facts.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, proof-log policy, or CTest registration.

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
gone from the delegated proof. No remaining `00140.c` residual was observed.
`test_after.log` contains the preserved proof output.
