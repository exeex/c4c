Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh The Current Variadic First Bad Fact

# Current Packet

## Just Finished

Step 1 - Refresh The Current Variadic First Bad Fact: refreshed the current `00204.c` variadic/HFA proof. The focused BIR, prepared-BIR, and generated AArch64 subset passed 15/15 with 0 failures, including `c_testsuite_aarch64_backend_src_00204_c`; no live in-scope idea 326 first bad fact is present in the refreshed proof.

## Suggested Next

Ask the plan owner to decide close/park/deactivate for idea 326 using the current green focused proof and the required close-gate policy.

## Watchouts

- Do not implement under this idea unless fresh evidence shows an in-scope HFA/floating, composite variadic call-boundary, or structured f128/q-register authority first bad fact.
- Treat prior fixed-formal, byval aggregate, stdarg cursor/format, MOVI, local/value-home, and frame/formal repairs as adjacent guardrails, not default scope.
- Do not weaken expectations, unsupported classifications, runner behavior, timeout policy, proof-log policy, or CTest registration.
- The refreshed proof did not expose a failure to classify with generated/dump evidence; current classification is first bad fact absent, not in-scope or out-of-scope failure handoff.

## Proof

Command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_dump_bir_00204_.*|backend_cli_dump_prepared_bir_00204_.*|backend_cli_dump_bir_focus_.*_00204|backend_cli_dump_prepared_bir_focus_.*_00204|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build up to date; focused subset passed 15/15 with 0 failures. `test_after.log` is the proof log path.
