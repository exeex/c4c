# 20000722 Local Memory Access Object Route Runbook

Status: Active
Source Idea: ideas/open/656_20000722_local_memory_access_object_route.md
Supersedes: parked active runbook for
`ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md`

## Purpose

Own the fresh `src/20000722-1.c` RV64 object-route
`unsupported_local_memory_access` blocker that prevents representative proof for
idea 648.

## Goal

Classify and repair, or split again, the first local-memory access owner that
stops the representative object route before call-argument disassembly.

## Core Rule

Do not use this route to change call-argument materialization. Idea 648 focused
coverage already proves the RV64 text-route consumer for
`arg.source_selection=local_frame_address_materialization`; this runbook owns
only the earlier object-route local-memory blocker.

## Read First

- `ideas/open/656_20000722_local_memory_access_object_route.md`
- `ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md`
- `ideas/closed/638_rv64_string_label_pointer_runtime_object_correctness.md`
- `ideas/closed/630_string_constant_local_memory_policy.md`

## Current Scope

- Representative integration surface: `src/20000722-1.c`.
- Fresh object-route diagnostics stop at `unsupported_local_memory_access`
  before object emission/disassembly.
- Focused idea 648 tests already pass for text-route call-argument
  frame-slot address materialization.

## Non-Goals

- Do not edit RV64 call-argument materialization for idea 648.
- Do not reopen broad string-constant local-memory admission or
  `StringConstantLabelPointer` policy.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `s2`, or `a0`.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting as progress.

## Working Model

The representative object route has an earlier local-memory access legality
blocker than the call-argument setup originally assumed by idea 648. The first
task is to name that blocker precisely. Only after the rejecting producer or
consumer is understood should an implementation packet be selected.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Start with fresh focused diagnostics for `src/20000722-1.c`; do not rely on
  the historical `mv a0,s2` disassembly as current evidence.
- Add focused coverage before changing object-route local-memory behavior.
- If the first owner is broad local-memory policy, create another split instead
  of absorbing that family here.
- If this route advances back to a call-argument mismatch, reactivate or hand
  back to idea 648 rather than fixing it here.

## Steps

### Step 1: Refresh The Object-Route Local-Memory Evidence

Goal: Identify the exact current `unsupported_local_memory_access` owner for
`src/20000722-1.c`.

Actions:

- Run focused prepared-BIR, BIR, RV64 object-route, and any available backend
  diagnostic dumps for `src/20000722-1.c`.
- Record the rejected memory value, access kind, owning block/instruction, and
  diagnostic producer in `todo.md`.
- Confirm whether the failure occurs before any call-argument emission decision
  is reachable.

Completion check:

- `todo.md` names the first unsupported local-memory access and the narrow
  lowering or object-emission boundary to inspect next.

### Step 2: Locate The Local-Memory Rejection Boundary

Goal: Find the smallest RV64 object-route branch or helper that rejects the
local-memory access.

Actions:

- Trace the diagnostic from object emission back to the prepared/BIR fact it
  consumes.
- Determine whether the access is an already-supported policy hole, a missing
  materialization, or a deliberate fail-closed rejection.
- Identify the narrow focused test that would fail on the current blocker.

Completion check:

- `todo.md` records the owned implementation surface, focused test target, and
  preserved fail-closed behavior.

### Step 3: Add Focused Local-Memory Route Coverage

Goal: Make the first blocker observable independently from the full GCC torture
row.

Actions:

- Add or update focused backend coverage for the rejected local-memory access
  shape.
- Assert the current diagnostic if the correct behavior is fail-closed, or
  assert the desired object-route behavior if a narrow repair is justified.
- Keep the coverage independent of `src/20000722-1.c` identifiers when
  possible.

Completion check:

- Focused coverage proves the first local-memory blocker and would catch a
  regression to guessing, broad admission, or source-file special casing.

### Step 4: Implement Or Split The Narrow Owner

Goal: Repair the local-memory object-route blocker only when the focused owner
is narrow and legitimate.

Actions:

- If Step 2 proves a narrow supported access is missing, implement that support
  and preserve negative/fail-closed cases.
- If Step 2 proves the owner is broad policy, create a split idea and park this
  runbook instead of broadening it.
- Do not change call-argument lowering in this step.

Completion check:

- Focused local-memory coverage passes after a fresh build, or lifecycle state
  records the split owner and parks this route.

### Step 5: Reprobe The Representative Row

Goal: Determine what `src/20000722-1.c` exposes after the local-memory blocker
is handled.

Actions:

- Rerun the focused local-memory proof and the RV64 object route for
  `src/20000722-1.c`.
- If object/disassembly reaches call-argument setup and shows a renewed
  materialization mismatch, hand back to idea 648.
- If the row advances to a distinct downstream owner, record that owner in
  `todo.md` and request lifecycle split or park.

Completion check:

- The lifecycle state identifies whether this idea can close, whether idea 648
  should reactivate, or whether another distinct owner should be split.
