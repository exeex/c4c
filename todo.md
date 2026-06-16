Status: Active
Source Idea Path: ideas/open/285_aarch64_llvm_path_fp128_vararg_codegen_crash_triage.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce And Reduce The Three Crash Families

# Current Packet

## Just Finished

Step 1 reproduced and reduced the three AArch64 LLVM-path clang backend crash
targets. The focused proof still fails all three cases with `[BACKEND_FAIL]`,
matching the baseline failure mode, and `test_after.log` contains fresh clang
19 crash stacks.

Crash evidence and reductions:

- `c_testsuite_src_00204_c`: clang crashes while compiling C4CLL IR for
  `@myprintf`, in `AArch64 Instruction Selection`; the full test stack includes
  `llvm::SelectionDAG::getTruncStore`. The relevant source construct is
  `myprintf(const char *, ...)` reading long-double HFA structs with
  `va_arg(ap, struct hfa31/hfa32/hfa33/hfa34)`. C4CLL IR defines those as
  `%struct.hfa31 = type { fp128 }` through `%struct.hfa34 = type { fp128,
  ... }`, then emits aggregate `va_arg ptr %lv.ap, %struct.hfa3N` and stores
  the aggregate into a local before loading `fp128` fields for `printf`.
  Reduced trigger: a standalone AArch64 IR function with only `llvm.va_start`,
  `va_arg ptr %ap, %struct.Q` where `%struct.Q = type { fp128 }`, and
  `store %struct.Q` reproduces an AArch64 instruction-selection clang crash in
  `@myprintf`.
- `llvm_gcc_c_torture_src_920625_1_c`: clang crashes in `RegBankSelect` on
  function `@va1`. The source reads `point` (`struct { double x, y; }`) from a
  variadic list. C4CLL IR calls `@va1(i32 4, double ..., double ...)` with each
  point flattened into two `double` varargs, but inside `@va1` retrieves the
  value with `va_arg ptr %lv.args, %struct._anon_0` where
  `%struct._anon_0 = type { double, double }`, then stores and field-loads the
  aggregate. Reduced trigger: standalone AArch64 IR containing only
  `llvm.va_start`, `va_arg` of `%struct.D = type { double, double }`, and a
  store reproduces the same `RegBankSelect` crash on `@va1`.
- `llvm_gcc_c_torture_src_pr44575_c`: clang crashes in `RegBankSelect` on
  function `@check`. The source reads `struct S { float a[3]; }` from a
  variadic list. C4CLL IR calls `@check(i32 1, float, float, float, float,
  float, float)` with each struct flattened into three `float` varargs, but
  inside `@check` retrieves with `va_arg ptr %lv.ap, %struct.S` where
  `%struct.S = type { [3 x float] }`, stores it, and loads float fields.
  Reduced trigger: standalone AArch64 IR with only `llvm.va_start`, `va_arg`
  of `%struct.F = type { [3 x float] }`, and a store reproduces the same
  `RegBankSelect` crash on `@check`.

Initial classification: these appear related but split into two backend-crash
families. The shared C4CLL shape is aggregate `va_arg` for AArch64 variadic
HFA-like structs while call sites pass the same aggregate as flattened scalar
varargs. The non-fp128 `{ double, double }` and `{ [3 x float] }` reductions
share the `RegBankSelect` family. The fp128 long-double HFA reduction is a
separate AArch64 SelectionDAG/fp128 path and may need a distinct repair or
explicit boundary after validating intended AArch64 ABI behavior.

## Suggested Next

Repair or further isolate the shared aggregate-vararg lowering rule: compare
the expected AArch64 variadic ABI representation for HFA-like aggregate
`va_arg` against C4CLL's current scalar-flattened call emission, starting with
the non-fp128 `RegBankSelect` family before deciding whether the fp128 path is
repairable or a narrow backend boundary.

## Watchouts

- Do not downgrade the tests to unsupported without reduced evidence and
  supervisor approval.
- Do not suppress stderr or hide clang backend crashes behind missing-binary
  runtime failures.
- Do not special-case the three filenames, symbols, or known crash functions.
- Keep C++ dependent casts and C aggregate function-pointer ABI work out of
  this plan.
- The tiny reductions show clang 19 can crash on aggregate `va_arg` IR alone;
  that does not by itself prove C4CLL may mark the tests unsupported. The C4CLL
  call-side/receive-side ABI mismatch must be checked first.
- The generated IR captures were transient and removed; durable evidence is in
  this packet summary plus `test_after.log`.

## Proof

Ran:

`cmake --build build_debug && ctest --test-dir build_debug -R '^(c_testsuite_src_00204_c|llvm_gcc_c_torture_src_920625_1_c|llvm_gcc_c_torture_src_pr44575_c)$' --output-on-failure 2>&1 | tee test_after.log`

Result: build was already up to date; focused CTest ran 3 tests and all 3
failed with `[BACKEND_FAIL]` clang backend crashes, as expected for this
triage packet. Proof log: `test_after.log`.
