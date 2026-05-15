# AArch64 Scalar Cast And Float Machine Nodes Runbook

Status: Active
Source Idea: ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md
Activated from: ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md

## Purpose

Lower supported AArch64 scalar cast and F32/F64 floating-point operations into
selected machine-node records and printer output using prepared register and
storage authority.

## Goal

Route simple integer casts, F32/F64 arithmetic, and typed float/integer
conversions through structured AArch64 machine nodes without reviving archived
accumulator conventions or merging register-bank policy into target codegen.

## Core Rule

AArch64 scalar cast and FP lowering must consume prepared value-home,
register-placement, type, and register-bank facts. Do not infer GPR/FPR homes
from rendered names, fixture names, or archived accumulator snippets.

## Read First

- `ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- prepared/register value-home and regalloc facts used by current scalar ALU
  lowering
- focused backend tests under `tests/backend/mir/`

## Current Targets

- `bir::CastInst` dispatch into selected AArch64 scalar cast records.
- Structured integer sign-extend, zero-extend, and truncate machine nodes.
- F32/F64 arithmetic records for add, sub, mul, and div.
- Typed float/integer conversion records with explicit GPR/FPR register-bank
  transition facts.
- Printer support that emits only from structured operands and prepared
  register authority.
- Fail-closed diagnostics for incomplete type, register, or register-bank
  facts.

## Non-Goals

- Do not rebuild archived accumulator-based `cast_ops.md` or `float_ops.md`
  surfaces.
- Do not merge GPR and FPR/SIMD allocation policy in AArch64 codegen.
- Do not lower F128 or binary128 through scalar F64 paths.
- Do not add i128 pair lowering, binary128 soft-float lowering, atomics,
  intrinsics, inline asm, or unrelated memory/call/frame behavior.
- Do not claim progress through expectation-only changes or fixture-shaped
  operation shortcuts.

## Working Model

Current AArch64 dispatch already lowers binary integer ALU operations through
structured records. This runbook extends the same selected-node discipline to
casts and scalar FP operations: inspect the prepared value and register facts
first, add the smallest complete selected records, then print only when the
record carries every operand and register-bank fact needed by the printer.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Start with an inspection packet that names the exact first code-changing
  target and proof subset.
- Add selected records before printer output.
- Preserve fail-closed diagnostics for unsupported types, missing register
  homes, missing bank transitions, and F128/binary128 cases.
- Treat named-case matching, expectation rewrites, fabricated registers, and
  F128-as-F64 shortcuts as route drift.
- For every code-changing packet, prove with a build plus the
  supervisor-chosen focused AArch64 scalar/FP backend subset. Escalate to
  broader backend validation after shared dispatch, prepared, or printer
  behavior changes across multiple operation families.

## Ordered Steps

### Step 1: Inspect Scalar Cast And FP Surfaces

Goal: identify the exact dispatch, record, prepared-register, and printer gaps
for integer casts and F32/F64 operations.

Primary targets:

- `bir::CastInst` classification and dispatch
- current AArch64 scalar ALU records and printer paths
- prepared value-home and regalloc facts for GPR and FPR/SIMD operands
- current diagnostics for unsupported casts, FP operations, and F128 cases

Actions:

- Trace simple integer cast, F32/F64 arithmetic, and float/integer conversion
  records from BIR through prepared state into AArch64 dispatch.
- Identify which structured operands each operation family needs:
  destination, source operands, source/destination type, width, signedness,
  register bank, and bank-transition facts.
- Compare required operands against existing prepared/register facts.
- Record the first implementation packet target and focused proof subset in
  `todo.md`.

Completion check:

- `todo.md` names whether execution can proceed to integer cast selection or
  must first split out a narrower prepared/register-bank authority blocker.

### Step 2: Select Simple Integer Cast Nodes

Goal: lower supported integer sign-extend, zero-extend, and truncate casts into
selected AArch64 machine records from prepared operands.

Actions:

- Add or extend selected records for simple integer cast operations.
- Consume prepared source and destination register facts directly.
- Preserve source width, destination width, signedness, and truncation/extend
  semantics as structured fields.
- Keep unsupported casts and incomplete register facts fail-closed.
- Add focused record/dispatch coverage for representative sign-extend,
  zero-extend, and truncate paths.

Completion check:

- Supported simple integer casts produce selected records with structured
  operands and missing facts still diagnose explicitly.

### Step 3: Print Simple Integer Cast Nodes

Goal: emit typed AArch64 cast instructions only from complete selected integer
cast records.

Actions:

- Add printer support for the selected integer cast subset.
- Print from structured destination/source registers, widths, and signedness.
- Preserve fail-closed diagnostics for unsupported or incomplete cast records.
- Add focused printer coverage for the supported integer cast subset.

Completion check:

- Supported integer casts print expected typed AArch64 instruction forms
  without recovering operands from names or text.

### Step 4: Select F32/F64 Arithmetic Nodes

Goal: lower F32/F64 add, sub, mul, and div through typed FP/SIMD selected
records.

Actions:

- Add selected records for F32/F64 arithmetic families.
- Consume prepared FP/SIMD source and destination register facts directly.
- Preserve operation kind, type width, register bank, and operand order as
  structured fields.
- Keep F128/binary128 and missing-bank cases fail-closed.
- Add focused record/dispatch coverage for F32 and F64 arithmetic paths.

Completion check:

- F32/F64 arithmetic produces typed selected FP/SIMD records, while F128 and
  incomplete register-bank cases remain unsupported with explicit diagnostics.

### Step 5: Add Typed Float/Integer Conversion Nodes

Goal: lower selected float/integer conversions with explicit register-bank
transition facts.

Actions:

- Add selected records for supported int-to-float, float-to-int, and
  float-width conversion paths.
- Consume prepared GPR and FPR/SIMD register facts for source and destination
  operands.
- Preserve signedness, source/destination type, width, rounding or conversion
  mode where available, and bank transition as structured fields.
- Keep missing bank-transition authority fail-closed instead of fabricating
  moves.
- Add focused record/dispatch coverage for representative conversion paths.

Completion check:

- Supported conversions produce selected records with explicit GPR/FPR
  transition facts, or the runbook records the exact prepared authority blocker.

### Step 6: Print FP Arithmetic And Conversion Nodes

Goal: print F32/F64 arithmetic and supported conversions from structured
selected records.

Actions:

- Add printer support for F32/F64 add, sub, mul, and div records.
- Add printer support for supported float/integer conversion records.
- Print only from structured operand, type, and register-bank fields.
- Preserve explicit diagnostics for unsupported F128/binary128 and incomplete
  records.
- Add focused printer coverage for arithmetic and conversion paths.

Completion check:

- Supported FP arithmetic and conversion cases print typed AArch64 instruction
  forms from selected records; unsupported cases remain fail-closed.

### Step 7: Validate And Summarize

Goal: prove semantic coverage without overfitting one fixture or weakening
unsupported cases.

Actions:

- Run the supervisor-chosen build and focused scalar/FP backend subset.
- Escalate to broader backend validation if shared dispatch, prepared
  register, or printer behavior changed across multiple operation families.
- Summarize supported cast, FP arithmetic, and conversion paths in `todo.md`.
- Record any remaining F128, binary128, i128, or register-bank blockers as
  separate lifecycle candidates instead of expanding this route silently.

Completion check:

- Supported scalar cast and F32/F64 paths pass through structured
  lowering/printing or fail with explicit unsupported diagnostics, and no
  supported case depends on name-shaped matching.
