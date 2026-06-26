# RV64 Variadic Aggregate Va Arg Cursor Stride Runbook

Status: Active
Source Idea: ideas/open/393_rv64_variadic_aggregate_va_arg_cursor_stride.md
Activated from: ideas/open/393_rv64_variadic_aggregate_va_arg_cursor_stride.md
Supersedes: ideas/open/387_rv64_object_route_same_module_sret_calls.md closed after Step 5 routed this later boundary.

## Purpose

Repair the later boundary exposed after idea 387: RV64 aggregate `va_arg`
consumption in a variadic callee advances the cursor by the aggregate object
size instead of by the ABI variadic GPR save-area slot stride.

## Goal

Make supported RV64 aggregate `va_arg` over register-passed variadic payloads
advance across the correct save-area slots, or fail closed with a precise
diagnostic owner.

## Core Rule

Start from callee-side `va_arg` cursor/layout evidence. Do not reopen
same-module sret call emission, caller publication, or previous `va_list`
publication owners without contradictory evidence.

## Read First

- `ideas/open/393_rv64_variadic_aggregate_va_arg_cursor_stride.md`
- `ideas/closed/387_rv64_object_route_same_module_sret_calls.md`
- `build/agent_state/387_step5_analysis.log`
- `build/agent_state/387_step5_920908-1.run.log`
- `build/agent_state/387_step5_920908-1.c4c-disasm.log`
- `build/agent_state/387_step5_920908-1.clang-disasm.log`
- `build/agent_state/387_step5_920908-1.qemu-cpu.log`
- `tests/c/external/gcc_torture/src/920908-1.c`

## Current Targets

- Representative: `tests/c/external/gcc_torture/src/920908-1.c`
- Callee-side RV64 aggregate `va_arg` lowering
- Variadic GPR save-area slot stride/layout
- Cursor advance after aggregate payload consumption
- Focused backend coverage for aggregate `va_arg` cursor behavior

## Non-Goals

- Do not reopen idea 387 same-module sret support.
- Do not reopen caller argument publication when qemu shows `a2=10` and
  `a3=20` at `f` entry.
- Do not reopen ideas 391 or 392 without fresh contradictory evidence.
- Do not implement a broad variadic or aggregate ABI redesign.
- Do not hard-code `920908-1.c`, callee `f`, registers, stack offsets, or the
  abort branch.

## Working Model

Step 5 of idea 387 showed that same-module sret call emission is complete for
the representative. The remaining abort occurs in `f`: c4c saves variadic
register payloads into 8-byte GPR save-area slots, reads the first 32-bit
aggregate payload correctly, then advances the cursor by 4 bytes. The next
read observes the high half of the first slot instead of the next slot. Clang
advances by 8 bytes for this register-saved aggregate shape.

## Execution Rules

- Capture current prepared/BIR/object evidence before selecting a repair.
- Keep aggregate object size distinct from ABI save-area slot stride.
- Preserve fail-closed behavior for ambiguous or unsupported aggregate
  `va_arg` shapes.
- Add focused backend coverage before or alongside implementation.
- Route any later representative boundary separately.

## Steps

### Step 1: Capture Aggregate Va Arg Cursor Evidence

Goal: identify the exact prepared/BIR/object facts for the aggregate `va_arg`
cursor and payload layout.

Primary target: `920908-1.c` callee `f`.

Actions:

- Inspect current prepared/BIR and object evidence for the two aggregate
  `va_arg` operations.
- Map the variadic save-area slots, cursor value before and after each
  aggregate read, aggregate object size, and ABI slot stride.
- Confirm whether the cursor advance is derived from aggregate size or from a
  missing/incorrect ABI slot fact.
- Record the exact fact gap and owner in `todo.md`.

Completion check: `todo.md` names the cursor source, first and second payload
slots, observed advance, expected advance, and likely repair owner.

### Step 2: Classify The Stride/Layout Rule

Goal: choose the narrow semantic rule for RV64 aggregate `va_arg` cursor
advance over register-saved variadic payloads.

Primary target: RV64 variadic `va_arg` lowering and prepared layout metadata.

Actions:

- Decide whether the cursor stride should come from RV64 ABI GPR slot width,
  aggregate classification, prepared layout facts, or a new explicit guard.
- Define supported and fail-closed variants.
- Compare clang behavior only to classify ABI semantics, not to clone
  instruction shape.
- Record the selected route and guards in `todo.md`.

Completion check: `todo.md` gives the executor an implementable rule and
guard set without re-deriving lifecycle scope.

### Step 3: Add Focused Backend Coverage

Goal: prove the selected aggregate `va_arg` stride behavior.

Primary target: focused RV64 backend tests/fixtures.

Actions:

- Add coverage for two register-saved aggregate `va_arg` reads where payloads
  occupy adjacent ABI GPR save-area slots.
- Assert that the cursor advances to the next slot, not just by aggregate
  object size.
- Add fail-closed coverage for unsupported or ambiguous aggregate layouts.
- Avoid testcase-shaped assertions tied to `920908-1.c` or callee `f`.

Completion check: focused coverage pins the current stride bug and passes only
when the second aggregate read reaches the next save-area slot.

### Step 4: Implement Narrow Aggregate Va Arg Stride Repair

Goal: repair the selected cursor/layout route without broad variadic rewrites.

Primary target: RV64 variadic `va_arg` lowering and direct helpers.

Actions:

- Use the selected prepared/layout facts to advance the cursor by the ABI
  save-area slot stride for supported aggregate payloads.
- Preserve existing scalar and non-register-backed `va_arg` behavior.
- Keep ambiguous shapes fail-closed.
- Run the delegated backend proof and record results in `todo.md`.

Completion check: focused backend tests pass and existing relevant backend
coverage remains green.

### Step 5: Rerun `920908-1.c` And Route The Next Boundary

Goal: verify the representative advances past the aggregate `va_arg` cursor
abort or route a later owner.

Primary target: `tests/c/external/gcc_torture/src/920908-1.c`.

Actions:

- Rerun the representative with the RV64 gcc_torture backend object case
  runner.
- Confirm whether the second aggregate `va_arg` reads the expected `20`.
- If a later boundary appears, record exact evidence and route it separately.
- Preserve the backend proof result required by the supervisor.

Completion check: `todo.md` records representative result, proof logs, and
either completion evidence for idea 393 or a clearly owned later boundary.
