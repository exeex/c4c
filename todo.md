Status: Active
Source Idea Path: ideas/open/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Local Operand Ownership

# Current Packet

## Just Finished

Step 1 localized the `00003.c` local-operand failure. The direct owner for
using incoming `w0` as the left subtraction operand is AArch64 prepared
caller-saved placement conversion: `src/backend/prealloc/target_register_profile.cpp`
assigns AArch64 `gpr:caller_saved#0` to `x13`, but
`src/backend/mir/aarch64/abi/abi.cpp::placement_register()` indexes
`abi::caller_saved_gp_registers()` where slot 0 is `x0`. The scalar operand
path in `src/backend/mir/aarch64/codegen/alu.cpp::make_prepared_scalar_operand()`
therefore reads prepared `%t0` as `w0` instead of the prepared `w13`.

Representative artifacts:

```text
storage %t0 register bank=gpr placement=gpr:caller_saved#0/w1 reg=x13
home %t0 value_id=0 kind=register reg=x13
```

Current `00003.c` AArch64 output:

```asm
main:
    sub w0, w0, #4
    ret
```

Passing/simple comparison: `00002.c` uses rematerializable/immediate operands,
not `gpr:caller_saved#0`, and emits:

```asm
main:
    mov w0, #3
    sub w0, w0, #3
    ret
```

The prepared `00003.c` also records `store_local`/`load_local` frame-slot
accesses for `x`; after the placement mismatch is repaired, the next proof may
expose a separate immediate-store/load materialization issue if the generated
code becomes a subtraction from uninitialized `w13`.

## Suggested Next

Execute Step 2: repair AArch64 local operand materialization by making prepared
register placement authority consistent for `gpr:caller_saved#N` between the
prealloc target profile and AArch64 codegen conversion. Then rerun:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_return_lowering$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003)_c$'; } 2>&1 | tee test_after.log
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
- Do not stop at removing the visible `sub w0, w0, #4` if the local value is
  still uninitialized. The prepared records include an immediate `store_local`
  and frame-slot `load_local`; if that becomes the next owner, fail or record
  it truthfully instead of special-casing `00003.c`.

## Proof

Ran the delegated proof:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_return_lowering$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003)_c$'; } 2>&1 | tee test_after.log
```

Result: command exited `8`. `backend_aarch64_return_lowering` passed.
`c_testsuite_aarch64_backend_src_00001_c` and
`c_testsuite_aarch64_backend_src_00002_c` passed through the AArch64 backend
runtime route. `c_testsuite_aarch64_backend_src_00003_c` failed at the current
owned layer with `[RUNTIME_NONZERO] exit=253`. Proof log: `test_after.log`.
