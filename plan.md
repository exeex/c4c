# AArch64 Intrinsic Machine Nodes Runbook

Status: Active
Source Idea: ideas/open/239_aarch64_intrinsic_machine_nodes.md
Activated From: ideas/open/239_aarch64_intrinsic_machine_nodes.md

## Purpose

Turn the AArch64 intrinsic coverage described by the source idea into structured backend facts, selected machine records, and printer output without recreating archived scratch-register snippets or x86-only fallbacks.

## Goal

Lower accepted AArch64 intrinsic families through explicit carriers and selected machine nodes, while unsupported or missing-fact routes fail closed with diagnostics.

## Core Rule

Intrinsic support must come from structured semantic/prepared/AArch64 records. Do not infer support from intrinsic spelling alone, accumulator conventions, or final assembly text.

## Read First

- `ideas/open/239_aarch64_intrinsic_machine_nodes.md`
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/bir/`
- `tests/backend/mir/`

## Current Repaired Scope

- Scalar FP unary F32/F64 `fabs` intrinsic machine-node selection and printer
  emission from complete structured carrier facts.
- Dependency split for CRC, vector memory, and vector operation carriers that
  do not yet have semantic/prepared authority.
- Explicit unsupported or incomplete-family diagnostics for barrier, cache,
  pause/hint, builtin address, CRC, vector, F128, and x86-only cases already
  covered by the completed packets.
- Fail-closed diagnostics for unsupported x86-only, missing-feature, missing-operand, or incomplete-carrier cases.

## Non-Goals

- Do not support all x86 SSE/AES/CLMUL intrinsics on AArch64.
- Do not rebuild archived `x0`/`w9`/`q0`/`q1` scratch-register conventions.
- Do not hide missing vector, CRC, or feature-gating facts behind printer text.
- Do not merge this route with inline asm, ordinary calls, frame lowering, binary128 helpers, atomics, or generic memory-node work.
- Do not add testcase-shaped intrinsic name shortcuts as proof of capability.

## Working Model

- Shared/BIR facts identify the intrinsic family, operation, feature requirements, operand/result types, and operand identities.
- Prepared facts preserve register, stack, and side-effect authority.
- AArch64 selection consumes complete prepared intrinsic facts into target machine records.
- The printer emits only from selected AArch64 intrinsic records with explicit operand/register authority.
- Unsupported families remain rejected or diagnosed rather than silently zero-filled.

## Execution Rules

- Keep each step small enough to validate with `cmake --build --preset default` plus the delegated backend subset.
- Add fail-closed tests before or with each new carrier boundary.
- Preserve existing scalar, call, vector, binary128, atomic, and inline-asm behavior unless the source idea explicitly requires intrinsic integration.
- If a family lacks upstream semantic authority, do not select or print it in
  this runbook. CRC/vector semantic and prepared carrier work is split to
  `ideas/open/241_aarch64_crc_vector_intrinsic_carriers.md`.
- Treat expectation downgrades, string-only intrinsic matching, and single-fixture hard-coding as route drift.

## Step 1: Inventory Intrinsic Authority And Fail-Closed Gaps

Goal: Map current intrinsic lowering and diagnostics before adding new machine-node support.

Primary targets:
- `src/backend/bir/`
- `src/backend/prealloc/`
- `src/backend/mir/aarch64/codegen/`
- `tests/backend/bir/`
- `tests/backend/mir/`

Actions:
- Inspect current BIR and prepared representations for intrinsic calls, runtime helper placeholders, side effects, vector operands, and scalar FP unary paths.
- Identify which intrinsic families already have structured semantic facts and which require dependency ideas.
- Add or tighten fail-closed coverage proving unsupported x86-only or incomplete intrinsic facts do not become fabricated AArch64 records.
- Record the first carrier boundary recommendation in `todo.md`.

Completion check:
- Backend proof passes.
- `todo.md` names the supported first family boundary and any missing upstream dependencies.
- No selected intrinsic records or printer output are added yet unless needed for fail-closed diagnostics.

## Step 2: Define Structured Intrinsic Carrier Facts

Goal: Add target-neutral or prepared carrier facts for intrinsic families that already have semantic authority.

Primary targets:
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Actions:
- Define carriers for accepted intrinsic families with family kind, operation, feature requirements, operand/result types, side-effect behavior, and operand identities.
- Preserve scalar FP unary, barrier/cache/hint, CRC, builtin-address, vector-memory, and vector-operation distinctions where source authority exists.
- Print structured carrier facts in BIR/prepared diagnostics without implying final AArch64 support.
- Keep incomplete or unsupported carriers fail-closed with explicit diagnostics.

Completion check:
- Carrier/printer tests prove structured facts preserve all fields needed by selection.
- Unsupported x86-only or partial facts still fail closed.
- No AArch64 assembly printing is added in this step.

## Step 3: Select Scalar FP Unary Intrinsic Machine Records

Goal: Consume complete prepared scalar FP unary F32/F64 `fabs` carriers into AArch64 selected records.

Primary targets:
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

Actions:
- Add selected records for scalar FP unary F32/F64 `fabs` with explicit register and feature authority.
- Reject missing feature facts, missing operands, unsupported operations, or non-AArch64 families.
- Avoid local scratch-register conventions unless the prepared/allocation layer published them structurally.

Completion check:
- Dispatch tests prove selected records preserve operation, features, operand/result registers, and side-effect facts.
- Unsupported or incomplete intrinsic facts produce explicit diagnostics.
- Barrier, cache, hint, builtin-address, CRC, vector, and printer emission remain out of this step unless already structurally ready.

## Step 4: Split Missing CRC And Vector Carrier Dependency

Goal: Preserve the scalar selected-record progress and park CRC/vector selection until complete carrier authority exists.

Primary targets:
- `todo.md`
- `ideas/open/241_aarch64_crc_vector_intrinsic_carriers.md`

Actions:
- Record that CRC, vector memory, and vector operation carriers are not complete in the active codebase.
- Keep the completed scalar F32/F64 `fabs` selected records as valid progress.
- Create or update the separate carrier dependency idea for CRC/vector semantic and prepared facts.
- Do not add CRC/vector selected records from intrinsic names, ordinary call plans, or archived scratch-register conventions.

Completion check:
- Dependency idea exists under `ideas/open/` with concrete scope and reviewer reject signals.
- `todo.md` points execution to the next responsible scalar-only step.
- Existing Step 4 proof logs remain monotonic and show no regression in the selected backend subset.

## Step 5: Print Scalar AArch64 Intrinsic Records

Goal: Emit AArch64 assembly text only from selected scalar FP unary intrinsic records.

Primary targets:
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

Actions:
- Add printer support for selected scalar FP unary F32/F64 `fabs` records supported by Step 3.
- Require explicit selected register, immediate, feature, and side-effect facts before printing.
- Keep unsupported x86-only and incomplete records fail-closed at printer boundaries.
- Do not print barrier/cache/hint, builtin-address, CRC, or vector intrinsic forms in this runbook unless a later lifecycle repair proves complete carriers and selected records exist.

Completion check:
- Printer tests prove final spelling comes from selected records and preserves operand/register contracts.
- Missing printer facts produce diagnostics rather than fabricated registers or zero-fill output.
- No inline-asm or atomic-template formatting is introduced.

## Step 6: Prove Route And Decide Closure

Goal: Prove the repaired scalar intrinsic route end to end and decide whether the active runbook should be retired, replaced, or kept open.

Primary targets:
- `tests/backend/bir/`
- `tests/backend/mir/`
- `ideas/open/239_aarch64_intrinsic_machine_nodes.md`

Actions:
- Add representative route coverage from scalar carrier facts through selected records and printer output for supported F32/F64 `fabs`.
- Confirm binary128 helpers remain delegated to the closed binary128 route and unsupported x86-only intrinsics remain rejected, trapped, or diagnosed by policy.
- Confirm CRC/vector work is tracked by `ideas/open/241_aarch64_crc_vector_intrinsic_carriers.md` rather than silently absorbed into this scalar-only runbook.
- Run the supervisor-delegated proof subset, then request lifecycle retirement or follow-on planning. Do not close the source idea unless its full scope is complete.

Completion check:
- Backend proof passes with no regressions.
- `todo.md` records supported scalar families, explicit unsupported families, and the separate CRC/vector dependency idea.
- Plan owner can determine whether this repaired runbook is complete while leaving idea 239 open for remaining source scope.
