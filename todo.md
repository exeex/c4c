Status: Active
Source Idea Path: ideas/open/285_aarch64_llvm_path_fp128_vararg_codegen_crash_triage.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Classify IR Validity And Ownership

# Current Packet

## Just Finished

Step 2 classified the three AArch64 aggregate `va_arg` crash targets. None of
the reduced cases is invalid generic LLVM IR: tiny variadic reductions using
`va_arg ptr %ap, %struct.D` for `{ double, double }`, `%struct.F` for
`{ [3 x float] }`, and `%struct.Q` for `{ fp128 }` all pass
`opt-19 -passes=verify -disable-output`. All three still crash Debian clang
19 during AArch64 object generation when compiled as IR.

Classifications and owners:

- `llvm_gcc_c_torture_src_920625_1_c`: C4CLL-owned avoidable AArch64
  LLVM-path lowering issue, not invalid IR. Generated C4CLL IR flattens each
  `point` vararg call operand into two `double` lanes but callee `@va1` reads
  with raw LLVM aggregate `va_arg ptr %lv.args, %struct._anon_0` and stores
  the aggregate. The verified minimal `{ double, double }` reduction crashes
  clang 19 in `RegBankSelect`, while clang 19's own AArch64 C frontend lowers
  the same `va_arg(args, point)` through explicit AAPCS64 `__va_list` fp-offset
  checks, register-save-area lane loads, overflow-area fallback, and memcpy.
  Owner/next action: repair C4CLL aggregate `va_arg` emission for AArch64 HFA
  doubles so the LLVM path emits explicit AAPCS64 va_list access or routes via
  the existing semantic aggregate helper path instead of raw LLVM aggregate
  `va_arg`.
- `llvm_gcc_c_torture_src_pr44575_c`: C4CLL-owned avoidable AArch64
  LLVM-path lowering issue, not invalid IR. Generated C4CLL IR flattens each
  `struct S { float a[3]; }` vararg into three `float` lanes at the call but
  callee `@check` retrieves it with raw LLVM aggregate
  `va_arg ptr %lv.ap, %struct.S`. The verified minimal `{ [3 x float] }`
  reduction crashes clang 19 in `RegBankSelect`; clang's AArch64 C frontend
  emits explicit `__va_list` fp-offset logic with 16-byte fp register-save
  slots for the three float lanes and an overflow fallback. Owner/next action:
  repair the same aggregate `va_arg` lowering surface for AArch64 float HFA
  arrays, preserving the existing scalar-lane call ABI and materializing the
  callee payload from the AAPCS64 va_list cursor.
- `c_testsuite_src_00204_c`: C4CLL-owned avoidable AArch64 LLVM-path lowering
  issue with an fp128-specific backend crash family, not invalid IR. Generated
  C4CLL IR already flattens normal HFA fp128 fixed arguments into scalar
  `fp128` lanes, but the variadic `myprintf` path uses raw aggregate `va_arg`
  for `%struct.hfa31` through `%struct.hfa34`. The verified minimal
  `{ fp128 }` aggregate `va_arg` reduction crashes clang 19 in AArch64
  instruction selection, while clang's AArch64 C frontend compiles the source
  and emits explicit AAPCS64 lowering instead of raw aggregate `va_arg`.
  Owner/next action: after the non-fp128 HFA repair shape is in place, extend
  or validate it for fp128 long-double HFA lanes. If clang 19 still crashes on
  correctly explicit fp128 AAPCS64 va_list lowering, only then consider a
  narrow external backend boundary for fp128 HFA variadic extraction.

Relevant C4CLL repair surface: frontend/LIR currently can print raw
`LirVaArgOp` as LLVM `va_arg` (`src/codegen/lir/lir_printer.cpp`), while the
BIR path already has an aggregate semantic helper
`llvm.va_arg.aggregate`, AArch64 HFA lane shape detection in
`src/backend/bir/lir_to_bir/calling.cpp`, and prepared AAPCS64 aggregate
`va_arg` access planning in `src/backend/prealloc/variadic_entry_plans.cpp`.
For these LLVM-path tests, the avoidable hostile construct is the final raw
LLVM aggregate `va_arg`, not the source-level C construct and not a need to
downgrade support status.

## Suggested Next

Implement a semantic AArch64 aggregate `va_arg` lowering for the LLVM output
path, starting with non-fp128 HFA payloads (`{ double, double }` and
`{ [3 x float] }`) and then applying the same ABI-shaped path to fp128
long-double HFA payloads.

## Watchouts

- Do not downgrade the tests to unsupported without reduced evidence and
  supervisor approval.
- Do not suppress stderr or hide clang backend crashes behind missing-binary
  runtime failures.
- Do not special-case the three filenames, symbols, or known crash functions.
- Keep C++ dependent casts and C aggregate function-pointer ABI work out of
  this plan.
- The raw aggregate `va_arg` crash is a real clang 19 AArch64 backend
  limitation candidate, but clang's own C frontend avoids that IR shape for
  these sources. Treat it as evidence for repairing C4CLL emission first, not
  as an acceptance-ready unsupported classification.
- The C4CLL caller/callee ABI split is general HFA-like aggregate vararg
  lowering: caller scalar-lane flattening is already present, but callee
  retrieval still uses raw LLVM aggregate `va_arg`.
- The fp128 case has a distinct SelectionDAG crash signature, so keep a
  separate residual-boundary decision after proving the explicit AAPCS64
  lowering strategy on non-fp128 cases.
- Tiny IR reductions were transient and removed; durable evidence is in this
  packet summary plus `test_after.log`.

## Proof

Ran:

`cmake --build build_debug && ctest --test-dir build_debug -R '^(c_testsuite_src_00204_c|llvm_gcc_c_torture_src_920625_1_c|llvm_gcc_c_torture_src_pr44575_c)$' --output-on-failure 2>&1 | tee test_after.log`

Result: build was already up to date; focused CTest ran 3 tests and all 3
failed with `[BACKEND_FAIL]` clang backend crashes, as expected for this
classification-only triage packet. Proof log: `test_after.log`.

Additional Step 2 evidence commands:

- `opt-19 -passes=verify -disable-output` accepted each tiny variadic aggregate
  `va_arg` reduction before backend codegen.
- `clang-19 -target aarch64-unknown-linux-gnu -x ir` reproduced the backend
  crashes for the tiny reductions: `RegBankSelect` for `{ double, double }`
  and `{ [3 x float] }`, AArch64 instruction selection for `{ fp128 }`.
- `clang-19 -target aarch64-unknown-linux-gnu -std=gnu89 -w -c` compiled all
  three original C sources, and `-S -emit-llvm` showed clang lowering these
  `va_arg` sites through explicit AAPCS64 va_list/register-save/overflow-area
  logic rather than raw LLVM aggregate `va_arg`.
