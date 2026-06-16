Status: Active
Source Idea Path: ideas/open/285_aarch64_llvm_path_fp128_vararg_codegen_crash_triage.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair Avoidable Or Invalid IR Shapes

# Current Packet

## Just Finished

Step 3 repaired the direct LLVM-output `va_arg` path for non-fp128 AArch64
HFA-like aggregates. `src/codegen/lir/hir_to_lir/call/vaarg.cpp` now
materializes classified non-fp128 HFAs from the AAPCS64 `__va_list` fp cursor:
it checks `fp_offset`, advances by one 16-byte fp register-save slot per lane,
loads register-save lanes into a temporary aggregate when registers are
available, otherwise copies the overflow-area payload using the HFA overflow
stride. The callee then loads the aggregate from that temporary instead of
emitting raw LLVM aggregate `va_arg`.

The repaired semantic surface covers:

- `llvm_gcc_c_torture_src_920625_1_c`: `{ double, double }` no longer routes
  through `LirVaArgOp` for the HFA callee extraction and the focused test now
  passes.
- `llvm_gcc_c_torture_src_pr44575_c`: `{ [3 x float] }` no longer routes
  through `LirVaArgOp` for the HFA callee extraction and the focused test now
  passes.

`src/codegen/lir/hir_to_lir/call/call.hpp` only gained the private helper
declaration. No expectation files, unsupported classifications, `plan.md`, or
source ideas were touched.

## Suggested Next

Have the supervisor decide whether to extend this explicit AAPCS64 HFA
materialization to fp128 long-double HFA `va_arg` payloads in a separate packet
or keep fp128 as the residual backend-boundary investigation.

## Watchouts

- This packet deliberately leaves fp128 HFA aggregate `va_arg` on the old raw
  aggregate path because the delegated repair was for the non-fp128 family and
  fp128 had a distinct SelectionDAG crash signature in Step 2.
- The direct harness does not retain `.ll` artifacts for these tests, but the
  repaired branch no longer constructs `LirVaArgOp` for non-fp128 HFAs; it
  emits explicit fp-offset/register-save/overflow LIR instead.
- The helper assumes the existing HFA classifier's lane order and byte offsets,
  matching the already-working variadic call flattening path.

## Proof

Ran:

`(cmake --build build_debug && ctest --test-dir build_debug -R '^(llvm_gcc_c_torture_src_920625_1_c|llvm_gcc_c_torture_src_pr44575_c)$' --output-on-failure) 2>&1 | tee test_after.log`

Result: build succeeded and the focused CTest subset passed 2/2:
`llvm_gcc_c_torture_src_920625_1_c` and
`llvm_gcc_c_torture_src_pr44575_c`. Proof log: `test_after.log`.

Supervisor-side validation after the 2-target proof:

`ctest --test-dir build_debug -R '^(c_testsuite_src_00204_c|llvm_gcc_c_torture_src_920625_1_c|llvm_gcc_c_torture_src_pr44575_c)$' --output-on-failure`

Result: `llvm_gcc_c_torture_src_920625_1_c` passed,
`llvm_gcc_c_torture_src_pr44575_c` passed, and
`c_testsuite_src_00204_c` still failed with `[BACKEND_FAIL]` in AArch64
instruction selection on `@myprintf`. This confirms fp128 remains the only
residual target after the non-fp128 repair.
