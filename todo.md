Status: Active
Source Idea Path: ideas/open/277_aarch64_backend_result_register_runtime_nonzero.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Return-Value Materialization

# Current Packet

## Just Finished

Step 2 repaired AArch64 scalar return-value materialization for the localized
result-register failure. `lower_scalar_instruction()` now retargets a selected
scalar result to the prepared `BeforeReturn` `FunctionReturnAbi` register when
that move exists, so a named computed integer return value does not stay in its
ordinary storage-plan GPR at `ret`.

Focused backend coverage was added to
`tests/backend/mir/backend_aarch64_return_lowering_test.cpp` for a
non-rematerializable register-backed scalar result whose storage home is `x19`
but whose prepared return ABI move targets call-result register `w0`.

Generated `00003.c` assembly advanced from:

```asm
main:
    sub w19, w0, #4
    ret
```

to:

```asm
main:
    sub w0, w0, #4
    ret
```

The original result-register owner layer is fixed: the computed subtraction is
now materialized into `w0` before `ret`, and the old `[RUNTIME_NONZERO] exit=1`
is gone.

A remaining truthful owner-layer failure is now exposed: `00003.c` still fails
as `[RUNTIME_NONZERO] exit=253` because the left operand is still the incoming
`w0` value rather than the source-local value `4` from `x = 4`. That belongs to
local store/load or operand materialization for the `%t0` input, not to return
result-register placement.

## Suggested Next

Localize the remaining `00003.c` AArch64 backend operand/local materialization
failure exposed after the result-register repair, without changing
c-testsuite expectations or route scripts. Reuse this proof command after the
next repair:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_return_lowering$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003)_c$'; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not change c-testsuite expectations, allowlists, unsupported
  classifications, or CTest contracts.
- Do not add filename-specific handling for `00003.c`.
- Do not route proof through LLVM IR fallback while claiming AArch64 backend
  progress.
- Keep runtime-runner and route-diagnostic work out of this plan unless the
  supervisor opens a separate lifecycle item.
- Do not fix this in the printer by special-casing `ret` or the `00003.c`
  instruction text. The repair needs to preserve the prepared ABI move
  semantics for returned integer values.
- Do not fold the remaining `[RUNTIME_NONZERO] exit=253` into the already-fixed
  return-register bug; it is a separate operand/local-value materialization
  issue.

## Proof

Command:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_return_lowering$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003)_c$'; } 2>&1 | tee test_after.log
```

Result: failed with exit code 8 because one of three selected smoke cases still
failed after the owned result-register repair. `cmake --build --preset default`
completed; `backend_aarch64_return_lowering` passed; `build-aarch64-scan`
configured and rebuilt `c4cll`; `00001.c` and `00002.c` passed the AArch64
backend `.s -> clang -x assembler -> executable -> runtime ->
expected-output` route. `00003.c` no longer fails with exit 1 and now emits
`sub w0, w0, #4; ret`, but still fails as `[RUNTIME_NONZERO] exit=253`.
Proof log: `test_after.log`.
