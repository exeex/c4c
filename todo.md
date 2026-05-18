Status: Active
Source Idea Path: ideas/open/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Local Operand Materialization

# Current Packet

## Just Finished

Step 2 test/contract alignment is complete for the AArch64 prepared
caller-saved placement conversion change. The regressed backend contracts now
align with the prealloc-owned caller-saved pool: `caller_saved#0` is the only
current AArch64 prepared caller-saved slot and resolves to `x13`/`w13`; tests
that prove placement authority over legacy register names now use that slot
instead of synthetic slots 1+.

Changed files:

- `tests/backend/mir/backend_aarch64_prepared_scalar_alu_records_test.cpp`
- `tests/backend/mir/backend_aarch64_prepared_scalar_cast_records_test.cpp`
- `tests/backend/mir/backend_aarch64_operand_resolution_test.cpp`
- `todo.md`
- `test_after.log`

The aligned backend subset passes, including the three regressed tests plus
prepared conversion and return-register coverage. The AArch64 runtime smoke
route remains at the next local store/load materialization owner layer.
Generated `00003.c` assembly still has no local store/load for `x = 4`:

```asm
main:
    sub w0, w13, #4
    ret
```

`00003.c` still fails at runtime with `[RUNTIME_NONZERO]` because `w13` remains
uninitialized; this run observed `exit=124`. The exact nonzero value is not the
owner signal. The stable signal is that the old incoming-`w0` placement bug is
gone and the artifact still lacks `str`/`ldr` for the prepared
`store_local`/`load_local`.

## Suggested Next

Execute the next packet against local memory assignment/reference
materialization: make the prepared immediate `store_local %lv.x, i32 4` and
frame-slot `load_local i32 %lv.x` produce real AArch64 storage traffic or an
equivalent semantically owned value before ALU emission. Then rerun:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_register_conversion|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_operand_resolution|backend_aarch64_return_lowering)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003)_c$'; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not change c-testsuite expectations, allowlists, unsupported
  classifications, or CTest contracts.
- Do not add filename-specific handling for `00003.c`.
- Do not route proof through LLVM IR fallback while claiming AArch64 backend
  progress.
- Preserve the idea 277 result-register repair: returned integer expression
  values must remain in `w0`/`x0` before `ret`.
- Keep runtime-runner and route-diagnostic work out of this plan unless the
  supervisor opens a separate lifecycle item.
- Keep ABI classification helpers such as `caller_saved_gp_registers()` as ABI
  vocabulary; prepared structured placement conversion now has its own
  prealloc-aligned caller-saved mapping.
- Do not treat `sub w0, w13, #4` as success: `w13` is still uninitialized until
  local store/load materialization is repaired.

## Proof

Ran the delegated proof:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_register_conversion|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_operand_resolution|backend_aarch64_return_lowering)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003)_c$'; } 2>&1 | tee test_after.log
```

Result: command exited `8`. `backend_aarch64_prepared_register_conversion` and
`backend_aarch64_prepared_scalar_alu_records`,
`backend_aarch64_prepared_scalar_cast_records`,
`backend_aarch64_operand_resolution`, and `backend_aarch64_return_lowering`
passed. `c_testsuite_aarch64_backend_src_00001_c` and
`c_testsuite_aarch64_backend_src_00002_c` passed through the AArch64 backend
runtime route. `c_testsuite_aarch64_backend_src_00003_c` remains past the old
`sub w0, w0, #4` / `[RUNTIME_NONZERO] exit=253` owner layer and failed in this
run with `[RUNTIME_NONZERO] exit=124` from the missing local store/load
materialization layer. Proof log: `test_after.log`.
