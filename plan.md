# AArch64 `inline_asm.md` Shard Redistribution Runbook

Status: Active
Source Idea: ideas/open/267_aarch64_inline_asm_markdown_shard_implementation_redistribution.md

## Purpose

Create a real AArch64 inline-asm owner for the current compiled backend route,
then remove the stale markdown shard.

## Goal

Reconcile `src/backend/mir/aarch64/codegen/inline_asm.md` into
`inline_asm.cpp` and `inline_asm.hpp` without turning the legacy reference
surface into an unbounded inline-asm feature port.

## Core Rule

This is an ownership and explicit-boundary route. Preserve supported
inline-asm behavior, keep deferred constraint/clobber/operand cases visible,
and do not port historical substitution machinery unless current prepared
facts and tests already authorize it.

## Read First

- `ideas/open/267_aarch64_inline_asm_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/inline_asm.md`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/BACKEND_CASE_BRINGUP_MATRIX.md`
- focused AArch64 backend tests that cover inline-asm printing, diagnostics,
  or explicit unsupported behavior

## Current Targets

- New `src/backend/mir/aarch64/codegen/inline_asm.cpp`.
- New `src/backend/mir/aarch64/codegen/inline_asm.hpp`.
- Current accepted inline-asm operand formatting, routing, diagnostics, or
  explicit deferred hooks if prepared machine facts already support them.
- Deletion of `src/backend/mir/aarch64/codegen/inline_asm.md` after durable
  valid content has been reconciled.

## Non-Goals

- Do not implement a full GCC inline-asm constraint system unless it is
  already supported by current prepared facts.
- Do not port the old reference substitution machinery wholesale.
- Do not change atomic lowering just because the legacy shard shared helper
  material with atomics.
- Do not rework the common MIR printer, assembler boundary, allocation
  contract, or unrelated machine instruction spelling.
- Do not redistribute neighboring AArch64 markdown shards.

## Working Model

`inline_asm.cpp` and `inline_asm.hpp` should become the live compiled boundary
for AArch64 inline-asm codegen. The markdown shard is reconciliation input, not
an implementation recipe: keep only behavior that matches the current prepared
MIR and machine-node route, route broad owners into the inline-asm owner where
appropriate, and leave unsupported forms explicit or fail-closed.

## Execution Rules

- Prefer small behavior-preserving ownership moves over broad semantic
  rebuilds.
- Treat legacy operand-substitution details as reference material unless the
  current route already exposes structured operands, homes, immediates,
  constraints, and clobbers.
- Keep unsupported inline-asm forms visible as explicit errors, diagnostics, or
  deferred handling; do not silently print placeholders.
- Do not add testcase-shaped shortcuts or named-template-only inline-asm
  lowering.
- Do not weaken, delete, or reclassify tests to claim inline-asm support.
- If implementation crosses broad codegen owners, prove the affected backend
  route with a focused build plus backend tests.

## Ordered Steps

### Step 1: Audit The Inline-Asm Shard And Current Route

Goal: Identify which facts in `inline_asm.md` can safely map to the current
compiled AArch64 backend route.

Primary target: `src/backend/mir/aarch64/codegen/inline_asm.md`

Actions:

- Inspect the markdown shard and classify each durable section as current,
  unsupported/deferred, or obsolete reference-only material.
- Search current AArch64 codegen owners for inline-asm routing, operand
  formatting, diagnostics, or explicit unsupported handling.
- Identify whether any accepted current inline-asm facts already belong in a
  new inline-asm owner.
- Record the audit result in `todo.md` before creating or moving code.

Completion check:

- `todo.md` names the current facts to keep, the unsupported facts to preserve
  as explicit fail-closed behavior, and any concrete routing or owner issue to
  fix.

### Step 2: Create The Inline-Asm Owner Boundary

Goal: Add `inline_asm.cpp` and `inline_asm.hpp` as the compiled owner for
AArch64 inline-asm codegen.

Primary target: `src/backend/mir/aarch64/codegen/inline_asm.cpp`

Actions:

- Create the inline-asm source and header using nearby AArch64 codegen owner
  conventions.
- Move or add only current accepted inline-asm formatting, routing, diagnostic,
  or deferred-boundary hooks that have prepared backend facts.
- Keep unsupported constraint, clobber, operand, and modifier forms explicit
  and fail-closed.
- Route broad dispatch or printer paths through the inline-asm owner instead
  of embedding inline-asm bodies in `machine_printer.cpp`, `dispatch.cpp`,
  `instruction.cpp`, or unrelated owners.

Completion check:

- `inline_asm.cpp` and `inline_asm.hpp` build, own the current inline-asm
  boundary, and do not claim unsupported full constraint/substitution support.

### Step 3: Delete The Stale Markdown Shard

Goal: Remove `inline_asm.md` after all durable valid content has either been
absorbed into compiled owners or explicitly rejected as unsupported/deferred.

Primary target: `src/backend/mir/aarch64/codegen/inline_asm.md`

Actions:

- Delete the markdown shard.
- Confirm no source list, docs index, classification index, bring-up matrix,
  or build/test reference still expects the deleted markdown file.
- Keep neighboring AArch64 markdown shards untouched.

Completion check:

- `inline_asm.md` is gone, and all repository references either point to the
  new compiled owner or no longer mention the stale shard.

### Step 4: Prove Focused Backend Behavior

Goal: Show that inline-asm ownership redistribution preserves supported
behavior and keeps unsupported behavior explicit.

Primary target: focused AArch64 backend proof.

Actions:

- Run the narrow proof command chosen by the supervisor for the implementation
  packet.
- Include focused backend tests that exercise affected AArch64 inline-asm
  printing, prepared machine-node routing, diagnostics, or explicit
  unsupported handling when available.
- Escalate to broader backend validation if routing changes affect shared
  dispatch, instruction, printer, or assembler behavior.

Completion check:

- Fresh proof is recorded in `test_after.log` by the executor, and `todo.md`
  summarizes the preserved supported behavior plus explicit unsupported
  behavior evidence.

## Completion Criteria

- `inline_asm.cpp` and `inline_asm.hpp` exist and own the current inline-asm
  boundary or explicit deferred handling.
- `src/backend/mir/aarch64/codegen/inline_asm.md` is deleted.
- Broad owners do not contain new inline-asm bulk implementation.
- Supported behavior is preserved, and unsupported behavior remains explicit.
- Focused backend proof covers the affected AArch64 codegen route.
