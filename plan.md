# RV64 Variadic Target LLVM-Route Facts Runbook

Status: Active
Source Idea: ideas/open/326_rv64_variadic_target_llvm_route_facts.md

## Purpose

Teach the target-aware LLVM-route/LIR path to produce RV64-correct variadic ABI facts for the minimal triage cases before semantic BIR, prepared ABI publication, or RV64 final emission consume those facts.

## Goal

Make RV64 LLVM-route output carry the required direct variadic integer-call extension facts and scalar `stdarg` pointer-cursor behavior while preserving the existing AArch64 structured `va_list` route.

## Core Rule

Repair target-owned LLVM-route/LIR fact production. Do not route this work through prepared ABI, final emission, fake libc behavior, expectation weakening, or named-testcase shortcuts.

## Read First

- `ideas/open/326_rv64_variadic_target_llvm_route_facts.md`
- `build/variadic_abi_triage/direct_variadic_printf.c`
- `build/variadic_abi_triage/stdarg_scalar_int.c`
- Existing target-aware LLVM-route, LIR, stdarg, and variadic-call lowering surfaces found by code search.

## Current Targets

- RV64 direct external variadic integer calls in `--codegen llvm` output.
- RV64 scalar `va_start` / `va_arg(ap, int)` lowering in `--codegen llvm` output.
- AArch64 structured `%struct.__va_list` lowering for the same scalar fixture.

## Non-Goals

- Full RV64 variadic runtime support.
- RV64 prepared ABI records for variadic callee entry, `va_arg`, or direct external calls beyond consuming already-correct target facts.
- RV64 final target emitter support for variadic calls or `stdarg`.
- Semantic BIR local-memory admission fixes for the scalar `stdarg` fixture.
- Broad c-testsuite registration for variadic cases.
- Fake libc stubs, special `printf` bodies, or c-testsuite symbol shortcuts.
- Copying AArch64 `va_list`, register-save-area, offset, slot, HFA, or save-area constants into RV64.
- Weakening unsupported diagnostics for backend routes that still lack target-correct facts.

## Working Model

- RV64 direct external variadic integer calls require integer extension facts in LLVM IR, including `signext` on the `i32` return and integer variadic operands where clang requires them.
- RV64 scalar `va_list` is a pointer cursor for the triage case: load the cursor, advance it by an 8-byte slot, store the advanced cursor, and load the scalar from the original cursor.
- AArch64 keeps its structured `%struct.__va_list` route and must not inherit the RV64 pointer-cursor shape.
- If no reusable LIR fact object exists for an item, introduce the smallest target-owned fact needed by LLVM-route output.

## Execution Rules

- Prefer semantic target rules over fixture-shaped matching.
- Keep each implementation step small enough to prove through `build -> narrow LLVM-route fixture checks`.
- Record unavailable lower-level LIR dump surfaces honestly in `todo.md` or `test_after.log`; prove through `--codegen llvm` when no lower-level dump exists.
- Do not rewrite expectations or downgrade diagnostics as evidence of capability progress.
- Escalate to supervisor review if the route requires semantic BIR, prepared ABI, or final RV64 emission changes before target-correct LLVM-route facts exist.

## Step 1: Locate Current RV64 Variadic Fact Boundaries

Goal: identify where direct external variadic calls, integer extension attributes, `va_start`, `va_list`, and scalar `va_arg` lowering are represented before LLVM IR emission.

Primary targets:

- Target-aware LLVM-route and LIR lowering code for function declarations, calls, builtin `va_start`, and `va_arg`.
- Existing AArch64 structured `va_list` handling.
- Existing RV64 target ABI or type-lowering helpers that can own extension and pointer-cursor facts.

Concrete actions:

- Search for variadic call, `printf`, `signext`, `va_start`, `va_arg`, `__va_list`, target ABI, and LLVM IR emission code paths.
- Generate or inspect current RV64 LLVM output for:
  - `build/variadic_abi_triage/direct_variadic_printf.c`
  - `build/variadic_abi_triage/stdarg_scalar_int.c`
- Compare against clang-observed facts recorded in the source idea.
- Choose the narrowest target-owned insertion points for Step 2 and Step 3.

Completion check:

- `todo.md` records the selected code surfaces, current observed RV64/AArch64 shapes, and the exact proof command the supervisor should delegate for the first implementation packet.

## Step 2: Publish RV64 Direct Variadic Integer Extension Facts

Goal: make RV64 direct external variadic integer calls emit the required LLVM-route extension facts without named-symbol shortcuts.

Primary targets:

- Function declaration and call lowering for external variadic integer functions on RV64.
- Target ABI helper or fact carrier chosen in Step 1.

Concrete actions:

- Add or preserve RV64 target facts for integer return extension attributes where clang requires them.
- Add or preserve RV64 target facts for integer variadic operands where clang requires them.
- Ensure the rule applies to direct external variadic integer calls generally, not only the literal `printf("%d\n", 7)` fixture.
- Keep non-RV64 behavior unchanged unless the existing helper already centralizes a target-neutral fact.

Completion check:

```sh
./build/c4cll --codegen llvm --target riscv64-linux-gnu build/variadic_abi_triage/direct_variadic_printf.c -o build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll
rg "declare signext i32 @printf|call signext i32 .*@printf|i32 noundef signext 7" build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll
```

The proof must not depend on a named `printf` special case or an expectation downgrade.

## Step 3: Model RV64 Scalar `stdarg` As Pointer Cursor

Goal: make RV64 scalar `va_start` and `va_arg(ap, int)` LLVM-route output use pointer-cursor behavior.

Primary targets:

- Target-specific `va_list` layout selection.
- Builtin `va_start` lowering.
- Scalar integer `va_arg` lowering for RV64.

Concrete actions:

- Select a single pointer cursor representation for RV64 scalar `va_list` in this route.
- Lower `va_arg(ap, int)` as cursor load, 8-byte cursor advance, cursor store, and `i32` load from the original cursor.
- Keep AArch64 on the structured `%struct.__va_list` route.
- Avoid copying AArch64 offset, save-area, HFA, or structured-field constants into RV64.

Completion check:

```sh
./build/c4cll --codegen llvm --target riscv64-linux-gnu build/variadic_abi_triage/stdarg_scalar_int.c -o build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll
rg "llvm.va_start|load ptr|store ptr|getelementptr inbounds i8, ptr .*, i64 8|load i32" build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll
```

The generated RV64 IR must no longer use the AArch64 structured `%struct.__va_list` lowering for this scalar fixture.

## Step 4: Prove AArch64 Structured `va_list` Remains Target-Specific

Goal: verify the RV64 pointer-cursor repair did not collapse AArch64 and RV64 into one variadic IR shape.

Primary targets:

- AArch64 LLVM-route output for `build/variadic_abi_triage/stdarg_scalar_int.c`.
- Shared target layout helpers touched by Step 3.

Concrete actions:

- Generate matching AArch64 LLVM-route output for the scalar `stdarg` fixture.
- Check that AArch64 still emits or uses the structured `%struct.__va_list` route.
- Compare the AArch64 and RV64 outputs at the `va_list` shape and `va_arg` extraction sites.

Completion check:

- `test_after.log` includes the AArch64 fixture command and a check proving structured `%struct.__va_list` is still AArch64-only.
- The proof documents any spelling differences that preserve the required target facts instead of weakening the contract.

## Step 5: Acceptance Checkpoint

Goal: collect the narrow proof and decide whether this idea is complete or whether a separate follow-up idea is needed for semantic BIR, prepared ABI, or final emission.

Concrete actions:

- Run the RV64 direct-call proof from Step 2.
- Run the RV64 scalar `stdarg` proof from Step 3.
- Run the AArch64 preservation proof from Step 4.
- Run a fresh build or compile proof selected by the supervisor for the changed code surface.
- Record any remaining unsupported backend diagnostics without weakening them.

Completion check:

- RV64 direct external variadic integer-call LLVM-route output carries clang-required extension facts.
- RV64 scalar `stdarg` LLVM-route output exposes pointer-cursor behavior.
- AArch64 still uses the structured `va_list` route.
- No implementation slice is accepted based on expectation rewrites, unsupported downgrades, helper renames alone, or named-case matching.
