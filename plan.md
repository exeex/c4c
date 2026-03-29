# AArch64 Global Int Pointer Difference Runbook

Status: Active
Source Idea: ideas/open/13_backend_aarch64_global_int_pointer_diff_plan.md
Activated from: ideas/open/13_backend_aarch64_global_int_pointer_diff_plan.md

## Purpose

Turn the next deferred AArch64 global-addressing seam into one executable backend slice.

## Goal

Lower `global_int_pointer_diff` through the backend-owned AArch64 asm path without widening into pointer round-trip or generic pointer-arithmetic work.

## Core Rule

Keep this runbook bounded to one defined global `int`-array pointer-difference comparison; split again instead of generalizing pointer arithmetic or address round-tripping.

## Read First

- `ideas/open/13_backend_aarch64_global_int_pointer_diff_plan.md`
- `ideas/open/__backend_port_plan.md`
- `tests/backend/backend_lir_adapter_tests.cpp`
- `tests/c/internal/InternalTests.cmake`
- `src/backend/aarch64/codegen/emit.cpp`

## Current Targets

- recognize one minimal `global_int_pointer_diff` LIR shape in the AArch64 emitter path
- emit backend-owned assembly for the bounded scaled pointer-difference comparison
- tighten synthetic and runtime coverage so this slice no longer accepts LLVM IR fallback

## Non-Goals

- `global_int_pointer_roundtrip`
- generic `ptrtoint` or `inttoptr` support
- broad global-address or local-memory refactors
- built-in assembler or linker work

## Working Model

- treat the completed `global_char_pointer_diff` slice as the immediate pattern for bounded address subtraction before adding one explicit scale step
- prefer one exact parser for the current LIR shape over introducing a generic pointer-difference framework
- keep synthetic backend coverage and one runtime case aligned to the same bounded seam

## Execution Rules

- add or tighten tests before changing `src/backend/aarch64/codegen/emit.cpp`
- compare the candidate runtime shape against the synthetic module builder instead of guessing ABI details
- if the slice requires pointer round-tripping, variable-width division beyond the exact `i32` element width, or multi-function support, stop and split a new idea

## Step 1: Lock The Bounded Slice

Goal: confirm the exact `global_int_pointer_diff` contract the backend should own.

Primary targets:

- `tests/backend/backend_lir_adapter_tests.cpp`
- `tests/c/internal/backend_case/global_int_pointer_diff.c`

Actions:

- inspect the synthetic module builder and the frontend-emitted runtime case for `global_int_pointer_diff`
- confirm the bounded seam is one global `int`-array base address, one constant element offset, one address subtraction, one divide by `4`, and one equality comparison
- document any mismatch in `plan_todo.md` instead of broadening the slice implicitly

Completion check:

- one exact LIR shape is identified as the implementation target
- the slice boundary excludes pointer round-tripping and generic element-width handling

## Step 2: Tighten Tests Before Codegen Changes

Goal: make the new asm contract observable before implementation.

Primary targets:

- `tests/backend/backend_lir_adapter_tests.cpp`
- `tests/c/internal/InternalTests.cmake`

Actions:

- update the synthetic AArch64 backend test so `global_int_pointer_diff` requires assembly-specific output instead of accepting LLVM IR fallback
- promote the runtime case to `BACKEND_OUTPUT_KIND=asm` only when the synthetic contract is explicit
- keep the assertions focused on the bounded scaled pointer-difference seam

Completion check:

- the synthetic test fails against LLVM IR fallback
- the runtime wiring is ready to exercise the asm path for this one case

## Step 3: Implement The Minimal AArch64 Lowering

Goal: emit assembly for the bounded scaled pointer-difference slice.

Primary targets:

- `src/backend/aarch64/codegen/emit.cpp`

Actions:

- add one narrow parser/emitter path for the `global_int_pointer_diff` module shape
- reuse the existing minimal-slice structure for symbol naming, ELF directives, and function prelude emission
- lower the comparison with the smallest instruction sequence that preserves the byte-granular subtraction and explicit divide-by-4 result

Completion check:

- `emit_module()` returns AArch64 assembly for the bounded `global_int_pointer_diff` slice
- the implementation stays local to the existing minimal-slice path instead of introducing generic pointer-arithmetic infrastructure

## Step 4: Validate And Record Follow-On Boundaries

Goal: prove the slice and preserve the next split boundary.

Actions:

- run the targeted backend adapter test and the `global_int_pointer_diff` runtime case
- run the relevant broader regression checks required by the execution prompt
- update `plan_todo.md` with what completed and note that `global_int_pointer_roundtrip` remains a separate follow-on

Completion check:

- the bounded synthetic and runtime validations pass on the asm path
- the next follow-on remains explicit instead of being absorbed into this runbook
