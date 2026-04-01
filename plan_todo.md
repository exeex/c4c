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
