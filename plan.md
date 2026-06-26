# RV64 Same-Module Sret Callee Home Publication Runbook

Status: Active
Source Idea: ideas/open/394_rv64_same_module_sret_callee_home_publication.md
Activated from: ideas/open/394_rv64_same_module_sret_callee_home_publication.md
Supersedes: ideas/closed/393_rv64_variadic_aggregate_va_arg_cursor_stride.md closed after Step 5 routed this later boundary.

## Purpose

Repair the later boundary exposed after idea 393: same-module RV64 callees can
receive an sret pointer in `a0` while prepared/local-memory use expects a
stack-homed `%ret.sret` pointer slot to have been published before return-store
use.

## Goal

Make supported same-module RV64 callee `sret_param` homes receive the incoming
ABI sret pointer before pointer-value local-memory use, or fail closed with a
precise diagnostic owner.

## Core Rule

Start from callee-side `%ret.sret` prepared/BIR/object evidence. Do not reopen
same-module caller memory-return emission, aggregate `va_arg` cursor stride, or
previous variadic publication owners without contradictory evidence.

## Read First

- `ideas/open/394_rv64_same_module_sret_callee_home_publication.md`
- `ideas/closed/393_rv64_variadic_aggregate_va_arg_cursor_stride.md`
- `build/agent_state/393_step5_analysis.log`
- `build/agent_state/393_step5_920908-1.prepared.log`
- `build/agent_state/393_step5_920908-1.bir.log`
- `build/agent_state/393_step5_920908-1.c4c-disasm.log`
- `build/agent_state/393_step5_920908-1.c4c-bin-disasm.log`
- `build/agent_state/393_step5_920908-1.qemu-cpu.log`
- `tests/c/external/gcc_torture/src/920908-1.c`

## Current Targets

- Representative: `tests/c/external/gcc_torture/src/920908-1.c`
- Callee-side same-module RV64 `sret_param` handling
- Incoming ABI sret pointer in `a0`
- Stack-homed `%ret.sret` publication before pointer-value return-store use
- Focused backend coverage for callee sret home publication

## Non-Goals

- Do not reopen idea 387 same-module caller memory-return support.
- Do not reopen idea 393 aggregate `va_arg` cursor stride.
- Do not reopen ideas 391 or 392 without fresh contradictory evidence.
- Do not implement a broad call ABI, prologue, or frame-layout redesign.
- Do not hard-code `920908-1.c`, callee `f`, registers, stack offsets, or the
  final store sequence.

## Working Model

Step 5 of idea 393 showed that aggregate `va_arg` cursor stride is complete for
the representative. The remaining segmentation fault occurs after both
aggregate checks pass. `main` passes the same-module sret address in `a0`, but
callee `f` later uses stack-homed `%ret.sret` as a pointer-value local-memory
base and loads it from `0(sp)` before any callee-side save/publication of the
incoming `a0` value into that home.

## Execution Rules

- Capture current prepared/BIR/object evidence before selecting a repair.
- Keep caller memory-return argument emission distinct from callee sret home
  publication.
- Preserve fail-closed behavior for ambiguous or unsupported sret parameter
  homes.
- Add focused backend coverage before or alongside implementation.
- Route any later representative boundary separately.

## Steps

### Step 1: Capture Sret Param Home Evidence

Goal: identify the exact prepared/BIR/object facts for the callee `%ret.sret`
home slot and incoming ABI sret pointer.

Primary target: `920908-1.c` callee `f`.

Actions:

- Inspect current prepared/BIR and object evidence for `%ret.sret`, its stack
  home, and the final pointer-value return store.
- Map incoming `a0`, any prologue saves/publications, the local-memory base,
  and the load from the `%ret.sret` home slot.
- Confirm whether the fault comes from an uninitialized callee home rather than
  caller-side sret address construction.
- Record the exact fact gap and owner in `todo.md`.

Completion check: `todo.md` names the incoming sret register, stack home slot,
missing publication/save point, return-store use, and likely repair owner.

### Step 2: Classify The Callee Home Publication Rule

Goal: choose the narrow semantic rule for same-module RV64 callee
`sret_param` home publication.

Primary target: RV64 function prologue/local-memory parameter publication.

Actions:

- Decide which prepared or ABI facts authorize saving incoming `a0` into the
  stack-homed `sret_param` slot.
- Define supported and fail-closed variants.
- Keep the rule separate from caller memory-return object emission.
- Record the selected route and guards in `todo.md`.

Completion check: `todo.md` gives the executor an implementable rule and guard
set without re-deriving lifecycle scope.

### Step 3: Add Focused Backend Coverage

Goal: prove the selected same-module callee sret home-publication behavior.

Primary target: focused RV64 backend tests/fixtures.

Actions:

- Add coverage for a callee `sret_param` stack home used by a pointer-value
  local-memory return store.
- Assert that the incoming ABI sret pointer is published into the home before
  the home is loaded.
- Add fail-closed coverage for unsupported or ambiguous sret home layouts.
- Avoid testcase-shaped assertions tied to `920908-1.c` or callee `f`.

Completion check: focused coverage pins the current home-publication bug and
passes only when the callee home is initialized from the incoming sret pointer.

### Step 4: Implement Narrow Sret Home Publication

Goal: repair the selected callee home-publication route without broad ABI or
frame rewrites.

Primary target: RV64 same-module callee `sret_param` publication and direct
helpers.

Actions:

- Use the selected prepared/ABI facts to publish incoming `a0` into supported
  stack-homed `sret_param` slots before pointer-value local-memory use.
- Preserve existing same-module caller memory-return emission and aggregate
  `va_arg` behavior.
- Keep ambiguous shapes fail-closed.
- Run the delegated backend proof and record results in `todo.md`.

Completion check: focused backend tests pass and existing relevant backend
coverage remains green.

### Step 5: Rerun `920908-1.c` And Route The Next Boundary

Goal: verify the representative advances past the current `%ret.sret`
home-publication segmentation fault or route a later owner.

Primary target: `tests/c/external/gcc_torture/src/920908-1.c`.

Actions:

- Rerun the representative with the RV64 gcc_torture backend object case
  runner.
- Confirm whether the callee return store uses the intended sret address and
  avoids the current segmentation fault.
- If a later boundary appears, record exact evidence and route it separately.
- Preserve the backend proof result required by the supervisor.

Completion check: `todo.md` records representative result, proof logs, and
either completion evidence for idea 394 or a clearly owned later boundary.
