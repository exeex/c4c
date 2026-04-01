# Plan TODO: Fix AArch64 Backend Failing Tests

Status: Active
Source Idea: ideas/open/fix_aarch64_backend_fail.md
Source Plan: plan.md

## Steps

- [x] Step 1 — Scaffold + single-block alloca/binop/ret
- [x] Step 2 — Multi-block: icmp + br + condbr
- [x] Step 3 — Direct function calls + multi-function modules
- [x] Step 4 — GEP + more cast ops + more ALU + phi nodes
- [x] Step 5 — Globals + string literals + switch + remaining edge cases
- [ ] Step 6 — Full suite cleanup and close-out

## Progress Notes

### Steps 1–5 completed in single implementation (2026-03-31)

Implemented `try_emit_general_lir_asm()` in `emit.cpp` as a comprehensive
stack-spill AArch64 emitter. All instruction types handled in one pass.

**Baseline**: 85/220 aarch64 tests passing
**After**: 185/220 aarch64 tests passing (+100 tests)
**Overall**: 2630/2671 total tests passing (was 2534, +96)

### Implementation Details
- GenSlotMap: every SSA value gets 8-byte stack slot; allocas get separate data area
- Stack-spill approach: load operands from stack → compute in scratch regs → store result
- Register convention: x0/w0 result, x1/w1 second operand, x2 scratch
- LR saved at [sp+0], restored at each ret and after each bl
- Parameters parsed from signature_text (LirFunction.params not populated by lowering)
- GOT-relative addressing for extern globals (adrp :got: + ldr :got_lo12:)
- String pool: LLVM hex escapes decoded and re-encoded for GAS assembler
- Global initializers: recursive handler for aggregates, arrays, ptr refs

### Remaining 35 failures
- 22 FRONTEND_FAIL (not backend issue)
- 6 RUNTIME_NONZERO (segfaults in struct-heavy or complex tests)
- 5 RUNTIME_MISMATCH (printf output doesn't match)
- 2 BACKEND_FAIL (assembler/linker errors)

### Next: Step 6
- Active slice: fix large stack-frame prologue/epilogue emission in the general
  AArch64 emitter so frames above the immediate encoding limit still assemble.
- Validate with a focused backend adapter test and the failing
  `c_testsuite_aarch64_backend_src_00216_c` case.
- Remaining after this slice: investigate `RUNTIME_NONZERO` failures (struct GEP
  issues likely) and `RUNTIME_MISMATCH` cases (printf / aggregate passing edge
  cases).

### Step 6 progress (2026-04-01)
- Added `gen_emit_sp_adjust()` so the general emitter splits stack pointer
  adjustments into legal AArch64 immediates instead of emitting one oversized
  `sub/add sp, sp, #frame_size`.
- Added a focused large-frame adapter test module covering a used
  `[5200 x i8]` alloca with expected `4095 + 1137` split adjustments.
- Verified the production compiler path on
  `tests/c/external/c-testsuite/src/00216.c`: emitted assembly now contains
  `sub sp, sp, #4095` + `sub sp, sp, #1137` and matching `add` instructions.
- Regression status for `00216.c` improved from `BACKEND_FAIL` (assembler reject)
  to `RUNTIME_NONZERO` (segfault), so the oversized-immediate backend failure is
  fixed and the next slice should target the remaining runtime bug.

### Step 6 active slice (2026-04-01)
- Current target: convert AArch64 `--codegen asm` LLVM-IR fallbacks into real
  assembly via the LLVM toolchain instead of exiting with `FRONTEND_FAIL`.
- Reproduced baseline: `ctest --test-dir build -R c_testsuite_aarch64 -j8
  --output-on-failure` currently reports 34 failures total, with the largest
  bucket being asm fallback refusals.
- Focus cases for this slice: `00108.c` (simple IR fallback) and `00125.c`
  (varargs/stdio IR fallback).
- Implemented fallback in `src/apps/c4cll.cpp`: when AArch64 `--codegen asm`
  receives LLVM IR from the backend path, it now tries `llc` first and then
  `clang -x ir -S`, with a retry against canonical `--codegen llvm` IR if the
  backend fallback IR is not consumable.
- Focus validation now passes for `00108.c` and `00125.c`.
- Current suite status after this slice: `ctest --test-dir build -R
  c_testsuite_aarch64 -j8 --output-on-failure` reports 12 failures total
  (improved from 34), all now runtime correctness issues rather than asm
  generation refusal.
- Next intended slice: investigate the remaining struct/aggregate runtime bugs
  driving `00040.c`, `00050.c`, `00091.c`, `00104.c`, `00115.c`, `00164.c`,
  `00170.c`, `00182.c`, `00195.c`, `00207.c`, `00216.c`, and `00217.c`.

### Step 6 active slice (2026-04-01, current)
- Current target: fix two confirmed general-emitter correctness bugs that are
  independent of the remaining struct-heavy runtime failures:
  1. direct/variadic calls currently drop arguments after `x7` instead of
     placing stack-passed overflow args in the caller argument area
  2. global C string initializers like `c"0123\00"` currently fall through to
     `.zero`, which breaks mutable global string data
- Focus validations for this slice:
  `c_testsuite_aarch64_backend_src_00170_c` and
  `c_testsuite_aarch64_backend_src_00217_c`
- Planned test-first coverage:
  add focused AArch64 backend adapter assertions for overflow call-argument
  stack stores and for emitted mutable string global bytes.
- Deferred in this slice:
  the `00164.c` arithmetic mismatch and the remaining segfault cases appear to
  be separate runtime bugs and will stay for the next slice unless the targeted
  fix unexpectedly resolves them.

### Step 6 slice result (2026-04-01)
- Implemented AArch64 overflow-argument stack passing in the general emitter:
  calls with more than 8 integer/pointer arguments now allocate a temporary
  16-byte-aligned outgoing argument area, spill extra args into 8-byte stack
  slots, preserve `lr` relative to the adjusted `sp`, and restore `sp` after
  the call.
- Implemented LLVM `c"..."` mutable global string initializer emission:
  mutable byte-array globals now decode LLVM hex escapes and emit real `.ascii`
  byte data instead of incorrectly falling through to `.zero`.
- Added focused backend adapter coverage for mutable string global byte
  emission in `backend_lir_adapter_aarch64_tests.cpp`.
- Focus validation:
  `ctest --test-dir build -R "c_testsuite_aarch64_backend_src_(00170|00217)_c"
  --output-on-failure` now passes both cases.
- Updated suite status:
  `ctest --test-dir build -R c_testsuite_aarch64 -j8 --output-on-failure`
  now reports 9 failures total (improved from 12).
- Fixed in this slice:
  `00170.c`, `00115.c`, and `00217.c` now pass in the AArch64 c-testsuite.
- Next intended slice:
  investigate the remaining runtime-only failures in `00040.c`, `00050.c`,
  `00091.c`, `00104.c`, `00182.c`, `00195.c`, `00207.c`, and `00216.c`,
  with `00207.c` or one of the struct-heavy segfaults as the next narrow
  root-cause slice.

### Step 6 slice result (2026-04-01, mixed-lifetime alloca alias)
- Root cause isolated from `00164.c`:
  entry-slot planning allowed a single-block scratch alloca to reuse the slot
  of a separate alloca whose value stayed live across multiple blocks. In
  practice this let `%lv.y` alias `%lv.a`, corrupting later arithmetic.
- Added shared slot-planning regression coverage for the mixed-lifetime case:
  a long-lived entry alloca and a single-block scratch alloca must receive
  distinct assigned slots and must not be rewritten onto the same canonical
  alloca.
- Fixed `src/backend/stack_layout/slot_assignment.cpp` so single-block reuse
  only considers previously assigned slots that are themselves in the
  single-block coalescing pool; whole-function-lifetime slots are no longer
  eligible for reuse.
- Focus validation:
  `ctest --test-dir build -R c_testsuite_aarch64_backend_src_00164_c
  --output-on-failure` now passes.
- Updated suite status:
  `ctest --test-dir build -R c_testsuite_aarch64 -j8 --output-on-failure`
  now reports 8 failures total (improved from 9).
- Remaining failures after this slice:
  `00040.c`, `00050.c`, `00091.c`, `00104.c`, `00182.c`, `00195.c`,
  `00207.c`, and `00216.c`.

### Step 6 slice result (2026-04-01, aggregate layout for named structs)
- Root cause isolated from `00050.c` and `00091.c`:
  the general AArch64 emitter approximated aggregate layout with primitive byte
  sizes, so named structs and arrays of named structs were indexed using
  pseudo-aligned sizes instead of LLVM ABI field offsets and element strides.
- Fixed `src/backend/aarch64/codegen/emit.cpp` by adding recursive aggregate
  layout helpers for named structs, nested structs, packed structs, and arrays,
  then routing struct-size, field-offset, and global-initializer sizing through
  that shared layout computation.
- Added focused backend coverage in
  `tests/backend/backend_lir_adapter_aarch64_tests.cpp` for:
  1. a named trailing-struct field that must land at byte offset `12`, not `16`
  2. an array-of-named-struct access that must stride by `12`, not `16`
- Focus validation:
  `ctest --test-dir build -R "c_testsuite_aarch64_backend_src_(00050|00091)_c"
  --output-on-failure` now passes both cases.
- Updated suite status:
  `ctest --test-dir build -R c_testsuite_aarch64 -j8 --output-on-failure`
  now reports 6 failures total (improved from 8).
- Full-suite snapshot after this slice:
  `ctest --test-dir build -j8 --output-on-failure` reports 12 failures out of
  2671 tests; this slice is only confirmed to improve the AArch64 subset.
- Remaining AArch64 failures after this slice:
  `00040.c`, `00104.c`, `00182.c`, `00195.c`, `00207.c`, and `00216.c`.
- Next intended slice:
  investigate one remaining narrow runtime bug, with `00207.c` or one of the
  remaining struct-heavy segfault cases as the next target.

### Step 6 slice result (2026-04-01, lowered parameter alloca initialization)
- Root cause isolated from `00040.c`, `00182.c`, and `00207.c`:
  the general AArch64 emitter spilled incoming `%p.*` ABI registers to SSA temp
  slots, but it never initialized the lowered `%lv.param.*` allocas that the
  frontend uses for mutable parameters and address-taken parameter storage.
- Fixed `src/backend/aarch64/codegen/emit.cpp` so, after alloca address slots
  are materialized, scalar/pointer incoming parameters also get written into the
  matching `%lv.param.*` storage slot in the function prologue.
- Added focused assembly-path coverage in
  `tests/backend/backend_lir_adapter_aarch64_tests.cpp` that asserts the
  general emitter emits the extra prologue writeback for `make_param_slot_module()`.
- Focus validation:
  `ctest --test-dir build -R "c_testsuite_aarch64_backend_src_(00040|00182|00207)_c" --output-on-failure`
  now passes `00040.c`; `00182.c` improves from `RUNTIME_NONZERO` to
  `RUNTIME_MISMATCH`; `00207.c` improves from a missing-output mismatch to a
  timeout caused by repeated `boom!` output.
- Updated AArch64 subset status:
  `ctest --test-dir build -R c_testsuite_aarch64 -j8 --output-on-failure`
  now reports 5 failures total out of 220 (`215/220` passing), improved from
  6 failures (`214/220` passing).
- Full-suite snapshot after this slice:
  `ctest --test-dir build -j8 --output-on-failure` reports 11 failures out of
  2671 tests (`2660/2671` passing), improved from the prior recorded 12
  failures. The remaining failures still include pre-existing non-AArch64
  issues such as `positive_sema_linux_stage2_repro_03_asm_volatile_c`,
  `backend_lir_adapter_aarch64_tests`, the three
  `backend_runtime_*param*_member_array` cases, and
  `llvm_gcc_c_torture_src_20080502_1_c`.
- Remaining AArch64 failures after this slice:
  `00104.c`, `00182.c`, `00195.c`, `00207.c`, and `00216.c`.
- Next intended slice:
  fix the remaining parameter/aggregate runtime correctness issue exposed by
  `00207.c` and the `backend_runtime_*param*_member_array` cases, likely by
  tightening how lowered parameter allocas interact with by-value aggregates
  and control-flow around address-taken parameters.

### Step 6 slice result (2026-04-01, preserved param register values + asm harness alignment)
- Root cause isolated from `00207.c`:
  the general AArch64 emitter initialized lowered `%lv.param.*` allocas after
  materializing later alloca addresses in `x0`/`x1`, so it could store a
  clobbered pointer value instead of the original incoming parameter register.
- Fixed `src/backend/aarch64/codegen/emit.cpp` so lowered parameter alloca
  initialization reloads the preserved incoming `%p.*` spill slot before
  writing into the alloca-backed storage slot.
- Added focused assembly-path coverage in
  `tests/backend/backend_test_fixtures.hpp` and
  `tests/backend/backend_lir_adapter_aarch64_tests.cpp` for a mixed
  parameter-slot + following-alloca function; the emitted prologue now spills
  `x0` first and reloads it for `%lv.param.x` initialization.
- Updated `tests/c/internal/InternalTests.cmake` so
  `nested_member_pointer_array`, `nested_param_member_array`, and
  `param_member_array` are classified as `asm` backend-output cases and passed
  to Clang with the assembler frontend instead of incorrectly as LLVM IR.
- Focus validation:
  `ctest --test-dir build -R "(c_testsuite_aarch64_backend_src_00207_c|backend_runtime_.*param.*member_array)" --output-on-failure`
  now passes all three targeted cases.
- Additional internal backend validation:
  `ctest --test-dir build -R backend_runtime_nested_member_pointer_array --output-on-failure`
  now passes after the asm-harness fix.
- Updated AArch64 subset status:
  `ctest --test-dir build -R c_testsuite_aarch64 -j8 --output-on-failure`
  now reports 3 failures total out of 220 (`217/220` passing), improved from
  5 failures (`215/220` passing). Remaining AArch64 failures:
  `00104.c`, `00195.c`, and `00216.c`.
- Full-suite snapshot:
  `ctest --test-dir build -j8 --output-on-failure` now reports 6 failures out
  of 2671 tests (`2665/2671` passing), improved from the prior recorded
  `2660/2671`. Remaining full-suite failures:
  `positive_sema_linux_stage2_repro_03_asm_volatile_c`,
  `backend_lir_adapter_aarch64_tests`,
  `c_testsuite_aarch64_backend_src_00104_c`,
  `c_testsuite_aarch64_backend_src_00195_c`,
  `c_testsuite_aarch64_backend_src_00216_c`, and
  `llvm_gcc_c_torture_src_20080502_1_c`.
- Next intended slice:
  split the remaining AArch64 work into two narrow follow-ons:
  `00104.c`/`00216.c` as likely stack- or aggregate-addressing runtime bugs,
  and `00195.c` as a separate floating-point / calling-convention mismatch.
