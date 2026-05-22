Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh The Current Variadic First Bad Fact

# Current Packet

## Just Finished

Step 1 refreshed the current `00204.c` variadic/HFA first-bad-fact evidence with the delegated BIR, prepared-BIR, focused BIR, and generated AArch64 proof subset. The proof is green, so no current in-scope first bad fact was found; classification: absent.

## Suggested Next

Supervisor can decide whether to close/deactivate this now-green evidence refresh or request a plan-owner route update for any remaining source-idea scope.

## Watchouts

- Do not implement under this idea unless fresh evidence shows an in-scope HFA/floating, composite variadic call-boundary, or structured f128/q-register authority first bad fact.
- Current evidence found no live owner in the delegated `00204.c` subset: all 15 selected tests passed, including `c_testsuite_aarch64_backend_src_00204_c`.
- Treat prior fixed-formal, byval aggregate, stdarg cursor/format, MOVI, local/value-home, and frame/formal repairs as adjacent guardrails, not default scope.
- Do not weaken expectations, unsupported classifications, runner behavior, timeout policy, proof-log policy, or CTest registration.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_dump_bir_00204_.*|backend_cli_dump_prepared_bir_00204_.*|backend_cli_dump_bir_focus_.*_00204|backend_cli_dump_prepared_bir_focus_.*_00204|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: passed; `test_after.log` contains the full delegated proof output.
