# Plan: Fix AArch64 Backend Failing Tests

Status: Active
Source Idea: ideas/open/fix_aarch64_backend_fail.md

## Purpose

135 out of 220 `c_testsuite_aarch64_backend_*` tests fail because the aarch64 backend
only handles specific pattern-matched IR "slices". When no pattern matches, the backend
falls back to printing LLVM IR text (not assembly). The test framework rejects this because
the output doesn't start with `.text`.

## Goal

Implement a general-purpose aarch64 assembly emitter so that all 220 c_testsuite
aarch64 backend tests pass.

## Core Rule

Add a `try_emit_general_lir_asm` function in `emit.cpp`. Use a **stack-spill-everything**
approach: every SSA value (alloca or temporary) gets a stack slot; every instruction
loads operands from stack, computes, stores result to stack. Simple, correct, no regalloc
needed.

## Read First

- `src/backend/aarch64/codegen/emit.cpp` — 4303 lines; general emitter goes here
- `src/codegen/lir/ir.hpp` — LirInst variant types: LirLoadOp, LirStoreOp, LirBinOp,
  LirCmpOp, LirCallOp, LirGepOp, LirCastOp, LirPhiOp, LirAllocaOp; terminators:
  LirRet, LirBr, LirCondBr, LirSwitch
- `src/codegen/lir/lir_printer.cpp` — shows how to read each LirInst field
- `tests/c/external/c-testsuite/RunCase.cmake` — checks output for `.text` pattern

## Non-Goals

- Register allocation / optimization (stack-spill is fine)
- Floating-point, SIMD, vector types
- Inline assembly (LirInlineAsmOp)
- Varargs (LirVaArgOp, LirVaStartOp)
- Intrinsics beyond abs/memset
- Preserving the existing specialized-slice paths (keep them as-is; new general path is the fallback)

## Working Model

### Stack Frame Layout

```
[sp+0]      : lr (link register / return address)
[sp+8..N]   : callee-saved regs if needed (keep it empty for stack-spill approach)
[sp+N..]    : alloca slots + SSA temp slots (8 bytes each for simplicity)
```

Frame is aligned to 16 bytes. All accesses use `str`/`ldr` to `[sp, #offset]`.

### Type Width Convention

| LLVM type | AArch64 width | register prefix |
|-----------|---------------|-----------------|
| i1, i8    | byte (1B)     | w  (zero-extend to 32b) |
| i16       | halfword (2B) | w  |
| i32       | word (4B)     | w  |
| i64, ptr  | dword (8B)    | x  |

All stack slots are 8 bytes wide; smaller types are zero-extended when loaded.

### Register Scratch Convention (single-instruction sequences)

- `w0`/`x0` — first (and usually only) result register
- `w1`/`x1` — second operand scratch
- `x2`, `x3` — third/fourth scratch for GEP chains or div/rem

### AArch64 Calling Convention

- Arguments: `w0`/`x0` through `w7`/`x7` (first 8 integer args)
- Return: `w0`/`x0`
- Caller-saved: x0–x15 (do NOT save; stack-spill covers them via reload from stack)
- Callee-saved: x19–x28 (we don't use them in the new path)
- Must save/restore `lr` (x30) around any `bl` calls

## Execution Rules

1. One slice per commit; run failing tests after each slice.
2. New general emitter goes at the **end** of `emit_module(LirModule)`, replacing the
   `print_llvm(prepared)` fallback — do not touch the existing slice patterns.
3. For each instruction type: add a handler, add at least one manual test in
   `backend_lir_adapter_aarch64_tests.cpp` OR verify via the existing c_testsuite run.
4. Do not break any currently-passing tests (check full `ctest -R c_testsuite_aarch64 -j8`).
5. Each step ends with: `ctest --test-dir build -R c_testsuite_aarch64 -j8` pass count improving.

## Steps

### Step 1 — Scaffold + single-block alloca/binop/ret

**Goal**: General emitter skeleton that handles the simplest case: single-block functions
with alloca, add/sub/mul, store immediate, ret.

**Primary target**: `emit.cpp` (add ~150 lines before `print_llvm` fallback)

**Actions**:
1. Add helper: `int lir_type_slot_size(std::string_view type_str)` → always returns 8.
2. Add helper: `bool lir_type_is_64bit(std::string_view type_str)` → true for i64/ptr.
3. Add `struct GenFrameInfo`: maps SSA name → stack offset; tracks next_offset and
   total frame_size.
4. Add `build_frame_info(const LirFunction&)` → scans alloca_insts (LirAllocaOp) and
   all block insts to collect every result SSA name → slot assignment.
5. Add `try_emit_general_lir_asm(const LirModule&) → std::optional<std::string>`:
   - Reject modules with `fp128`, vector types, LirInlineAsmOp, LirVaArgOp (return nullopt).
   - For each function: emit `.text`, `.globl`, label, prologue (sub sp, sp, #frame_size;
     str x30, [sp, #0]).
   - Emit alloca_insts: only LirAllocaOp; skip (no instruction emitted; slot already reserved).
   - Emit block instructions: only LirStoreOp (store immediate to alloca), LirBinOp
     (add/sub/mul), LirLoadOp (load from alloca or immediate ptr).
   - Emit terminator: only LirRet (ldr w0/x0 from slot; ldr x30; add sp; ret).
   - Reject (return nullopt) if unsupported instruction encountered.
6. Wire in: at the end of `emit_module(LirModule)`, before `print_llvm(prepared)`:
   `if (auto asm = try_emit_general_lir_asm(prepared)) return *asm;`

**Completion check**: Build passes; `ctest -R c_testsuite_aarch64 -j8` shows at least
the same number as before (regression-free). Manually verify at least one test using
`/workspaces/c4c/build/c4cll --codegen asm` succeeds for a simple input.

---

### Step 2 — Multi-block: icmp + br + condbr

**Goal**: Handle multi-block functions with icmp comparisons and conditional/unconditional branches.

**Actions**:
1. Add `LirCmpOp` handler in per-instruction emission:
   - `eq/ne/slt/sle/sgt/sge/ult/ule/ugt/uge` predicates
   - Emit: `ldr w0, [sp, #lhs]; ldr w1, [sp, #rhs]; cmp w0, w1; cset w0, <cond>; str w0, [sp, #result]`
   - For i64 operands use x registers.
2. Add multi-block emission: emit each block label, then its insts, then its terminator.
3. Add `LirBr` handler: `b .block_label`
4. Add `LirCondBr` handler:
   - `ldr w0, [sp, #cond]; cbnz w0, .true_label; b .false_label`
5. Add `LirCastOp` handler for `ZExt` and `SExt`:
   - `zext i1 %v to i32` → load w0, `and w0, w0, #1`, store
   - `sext i32 %v to i64` → load w0, `sxtw x0, w0`, store x0 (8 bytes)
6. Handle literal operand in binop/cmp (e.g. `add i32 %t0, 1` where rhs is literal "1"):
   use `mov w1, #imm` or `mov x1, #imm` before operation.

**Completion check**: `ctest -R c_testsuite_aarch64 -j8` shows significant improvement
(target: 40+ more tests pass vs. baseline 85).

---

### Step 3 — Direct function calls + multi-function modules

**Goal**: Handle `call i32 @func(...)` and multi-function modules.

**Actions**:
1. Before calling any function:
   - Save current live caller-saved values: since we spill everything to stack and reload
     from stack after the call, no explicit register saves needed (stack slots survive the call).
   - Save `lr` is done at function entry; for inner calls, lr is overwritten by `bl` so
     restore is needed. Strategy: save `lr` to a dedicated slot at function entry, restore
     before `ret`, and also restore after each `bl`:
     ```
     // After each bl:
     ldr x30, [sp, #0]   // restore lr from frame slot
     ```
   - Actually: save lr at prologue, restore at each ret, AND at each bl restore it right
     after. Alternatively, for each `bl`, use a scratch area.
   - Simpler: just re-save lr to its slot before each call and restore after.
     Emit: `str x30, [sp, #0]` before each `bl`, then `ldr x30, [sp, #0]` after.
2. Load call arguments into x0–x7 per AArch64 ABI:
   - Parse args_str from LirCallOp to extract typed arguments
   - Load each arg from its stack slot into x0, x1, x2... (up to 8 args)
3. Emit `bl @callee` for direct calls.
4. After call: store `w0`/`x0` (return value) to result slot.
5. Handle extern declare functions (no body): just emit nothing for them.
6. Handle void calls (no result): don't store w0.

**Completion check**: Tests with external function calls and multi-function modules pass.
Target: 60+ more tests pass vs. baseline.

---

### Step 4 — GEP + more cast ops + more ALU + phi nodes

**Goal**: Handle pointer arithmetic, more casts, and phi nodes.

**Actions**:
1. `LirGepOp` handler:
   - Compute byte offset from type size and index.
   - Common pattern: `getelementptr [N x T], ptr %base, i64 0, i64 idx`
     → `ldr x0, [sp, #base]; ldr x1, [sp, #idx]; mul x1, x1, #elem_size; add x0, x0, x1; str x0, [sp, #result]`
   - Two-index form: first index = outer array stride, second = element offset.
   - Struct GEP: `getelementptr %struct.T, ptr %p, i32 0, i32 field_idx`
     → compute byte offset of field_idx statically.
2. More `LirCastOp` kinds:
   - `PtrToInt` (ptr → i64): copy 8-byte slot directly, rename
   - `IntToPtr` (i64 → ptr): copy 8-byte slot directly, rename
   - `Trunc` (i64 → i32 or i32 → i8/i16): load, mask/truncate, store
3. `LirPhiOp` handler:
   - Phis must be lowered to copies at the end of predecessor blocks.
   - Pre-pass: for each block, collect phi insts. For each predecessor, at the end of
     that predecessor's instruction list (before terminator), insert copy of phi operand
     to a dedicated phi-resolve slot. At the phi's block entry, copy from resolve slot to
     result slot.
   - Simpler: create a per-phi "incoming value" slot. At the predecessor's terminator,
     emit `ldr w0, [sp, #incoming_val]; str w0, [sp, #phi_resolve_slot]`.
     Then in the phi's block, `ldr w0, [sp, #phi_resolve_slot]; str w0, [sp, #phi_result_slot]`.
4. More `LirBinOp` opcodes: `sdiv`, `udiv`, `srem`, `urem`, `and`, `or`, `xor`, `shl`,
   `lshr`, `ashr`.
   - For srem/urem: `sdiv x2, x0, x1; msub x0, x2, x1, x0` (or udiv/msub).
5. Handle `xor i1 %v, true` (boolean NOT): `ldr w0, [sp, #v]; eor w0, w0, #1; str w0, [sp, #result]`.
6. Handle `LirSelectOp` (select i1 cond, val1, val2):
   - `ldr w0, [sp, #cond]; cbnz w0, .sel_true; ldr x0, [sp, #false_val]; b .sel_end; .sel_true: ldr x0, [sp, #true_val]; .sel_end: str x0, [sp, #result]`

**Completion check**: Tests with struct access, phi nodes, boolean NOT pass.
Target: 85+ more tests pass vs. baseline (total ≥170).

---

### Step 5 — Globals + string literals + switch + remaining edge cases

**Goal**: Handle global variable references, string literal globals, switch statements, and
any remaining blockers identified from step 4's non-passing tests.

**Actions**:
1. Global variable references:
   - `@g` in load/store: use `adrp x0, g; add x0, x0, :lo12:g; ldr w1, [x0]` pattern.
   - Global declarations in asm output: emit `.bss` / `.data` sections as needed.
2. String literal globals:
   - `@.str0 = private unnamed_addr constant [N x i8] c"..."`
   - Emit in `.rodata` section.
   - `getelementptr [N x i8], ptr @.str0, i64 0, i64 0` → `adrp x0, .str0; add x0, x0, :lo12:.str0`
3. `LirSwitch` terminator:
   - Simple case: emit a chain of `cmp`+`beq` for each case value, `b default` at end.
4. Extern declarations: emit just the label reference (no `.globl` or body).
5. Run full test suite; check what's still failing; fix remaining patterns.

**Completion check**: `ctest -R c_testsuite_aarch64 -j8` ≥ 200/220 tests pass.
Run `ctest --test-dir build -j8` to confirm no regressions in the broader suite.

---

### Step 6 — Full suite cleanup and close-out

**Goal**: Fix any remaining failing tests identified in step 5; ensure full regression
cleanliness.

**Actions**:
1. Categorize remaining failures.
2. Fix remaining blockers (pattern-by-pattern).
3. Run `ctest --test-dir build -j8` — all 2123+ tests must still pass.
4. Commit; update ideas/open, move to ideas/closed.

**Completion check**: All 220 c_testsuite_aarch64 tests pass AND 2123/2123 overall suite still passes.
