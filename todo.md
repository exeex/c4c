Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh The Current Variadic First Bad Fact

# Current Packet

## Just Finished

Step 1 - Refresh The Current Variadic First Bad Fact: reran the delegated `00204.c` variadic/HFA focused proof. The current BIR, prepared-BIR, and generated AArch64 executable coverage is green, so no live `00204.c` first bad fact remains in the in-scope HFA/floating, composite variadic call-boundary, or structured f128/q-register authority buckets for idea 326.

## Suggested Next

Lifecycle handoff should follow. There is no Step 1 implementation packet to run against `00204.c` unless the supervisor has fresh failing evidence outside this focused subset.

## Watchouts

- Do not implement under this idea from `00204.c`; the refreshed focused subset has no current in-scope first bad fact.
- Treat prior fixed-formal, byval aggregate, stdarg cursor/format, MOVI, local/value-home, and frame/formal repairs as adjacent guardrails, not default scope.
- Do not weaken expectations, unsupported classifications, runner behavior, timeout policy, proof-log policy, or CTest registration.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_dump_(bir|prepared_bir)(_00204_.*|.*00204)|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: passed, 15/15 tests. The exact proof output is captured in `test_after.log`.
