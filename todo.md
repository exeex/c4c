Status: Active
Source Idea Path: ideas/open/370_aarch64_short_circuit_control_publication_regression.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Short-Circuit Regression Boundary
你該做test baseline review了

你該做baseline sanity check了

# Current Packet

## Just Finished

Activated a focused lifecycle plan for the close-time full-suite regression:
`c_testsuite_aarch64_backend_src_00196_c` is newly failing after idea 366 while
`c_testsuite_aarch64_backend_src_00112_c` is resolved.

## Suggested Next

Execute Step 1: localize the first AArch64 backend boundary where
`fred() && (1 + joe())` evaluates the right-hand `joe()` call after `fred()`
already returned `0`.

Suggested initial proof/localization command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00112_c)$' > test_after.log 2>&1
```

## Watchouts

Keep the repair scoped to the newly introduced short-circuit/control
publication regression. Do not expand into unrelated backend inventory or
weaken any external test contract. Preserve the `00112` fix.

## Proof

Activation-only lifecycle change. No implementation proof was run by the plan
owner.
