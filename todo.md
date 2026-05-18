Status: Active
Source Idea Path: ideas/open/277_aarch64_backend_result_register_runtime_nonzero.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Return Result Register Ownership

# Current Packet

## Just Finished

Step 1 localized the AArch64 return-expression result-register owner.

Owner function/stage:
`src/backend/mir/aarch64/codegen/alu.cpp` scalar lowering plus
`src/backend/mir/aarch64/codegen/returns.cpp` return lowering. The smallest
semantic repair point is to make the scalar result that is immediately returned
honor the prepared `BeforeReturn` `FunctionReturnAbi` move to call-result
register `w0`, or equivalently make return lowering materialize/move an
emitted non-ABI scalar register into the prepared return ABI register before
`ret`.

Key localization facts:

- `00001.c` has no named return value; generated AArch64 is `mov w0, #0; ret`.
- `00002.c` has `%t0 = bir.sub i32 3, 3`, but prepared storage classifies `%t0`
  as a rematerializable immediate `0` and carries a `before_return` move to
  `placement=gpr:call_result#0/w1 reg=x0`; generated AArch64 is
  `mov w0, #3; sub w0, w0, #3; ret`.
- `00003.c` prepares `%t0` in `x13` and return value `%t1` in callee-saved
  storage `x20`, with a `before_return` move from `%t1` to
  `placement=gpr:call_result#0/w1 reg=x0`.
- The generated `00003.c` assembly is still:

```asm
main:
    sub w19, w0, #4
    ret
```

  so the computed return value remains outside `w0` at `ret`.
- `make_prepared_scalar_alu_record()` uses
  `make_prepared_scalar_result_register_operand()` to select the prepared
  storage-plan result register for `%t1` before the fallback path can consult
  `find_return_abi_register()`.
- `lower_scalar_instruction()` records that emitted `%t1` register in
  `BlockScalarLoweringState`; `make_return_record()` then prefers
  `find_emitted_scalar_register()` and `classify_return_value_print_form()`
  treats any register operand as `PrimaryReturn`, so no move to `w0` is emitted
  before `ret`.

## Suggested Next

Execute Step 2 by repairing AArch64 return-value materialization at the scalar
result/return ABI boundary above. Add focused backend coverage so a named
integer return value with a storage-plan GPR distinct from `w0` is returned
through `w0`, then run this exact proof:

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
- `tests/backend/mir/backend_aarch64_return_lowering_test.cpp` already has
  return-selected scalar fixtures that exercise `FunctionReturnAbi`; extend
  that pattern for a non-rematerializable/storage-register case instead of
  inventing a c-testsuite-shaped test.

## Proof

Command:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003)_c$'; } 2>&1 | tee test_after.log
```

Result: failed with exit code 8 because one of three selected tests failed.
`00001.c` and `00002.c` passed the AArch64 backend `.s -> clang -x assembler
-> executable -> runtime -> expected-output` route. `00003.c` failed at
`[RUNTIME_NONZERO] ... exit=1`. Proof log: `test_after.log`.
