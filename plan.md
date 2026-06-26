# RV64 Object Route Byval Aggregate Parameter Homes Runbook

Status: Active
Source Idea: ideas/open/370_rv64_object_route_byval_aggregate_param_homes.md

## Purpose

Continue the RV64 object-route aggregate parameter-home work from the idea 354
follow-up chain. The current representative stops at a precise byval aggregate
parameter-home boundary after earlier variadic entry admission work advanced
the case to this target ABI gap.

Goal: lower or precisely route RV64 byval aggregate parameter homes represented
in prepared stack slots without guessing ABI state.

## Core Rule

Consume explicit prepared ABI parameter-home, frame, and value-home facts. Do
not key behavior on `src/20030914-2.c`, exact source spelling, function names,
block labels, value ids, instruction indexes, physical registers, stack offsets
not published by prepared metadata, object addresses, or log text.

## Read First

- `ideas/open/370_rv64_object_route_byval_aggregate_param_homes.md`
- `todo.md`
- `ideas/closed/363_*` if historical parameter-home boundary context is needed
- `ideas/closed/367_*` if helper-free variadic entry context is needed
- `build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log` when present

## Current Targets

- Primary representative: `src/20030914-2.c`
- Current diagnostic: `unsupported_byval_param_home`
- Current owner: RV64 object-route byval aggregate parameter homes in prepared
  stack slots.
- Desired outcome: either admit the first safe semantic class of byval
  aggregate parameter-home movement, or keep a more precise fail-closed
  boundary when required prepared facts are missing.

## Non-Goals

- Do not reopen helper-free variadic entry admission from idea 367.
- Do not implement scalar or aggregate `va_arg` helper lowering; route that to
  idea 371.
- Do not implement non-byval non-register parameter homes; route that to idea
  374 unless the audit proves the same parameter-home contract is required.
- Do not broaden into aggregate transport, global memory, full ABI redesign, or
  source-layout reconstruction unrelated to byval parameter homes.
- Do not infer aggregate bytes from assumed stack layouts not published by the
  prepared parameter-home contract.
- Do not add testcase-name handling for `src/20030914-2.c`.
- Do not weaken runtime expectations, external allowlists, supported-path
  contracts, or focused backend coverage.

## Working Model

- Prepared module state already distinguishes byval aggregate parameter homes
  from scalar register homes.
- RV64 object emission currently rejects byval aggregate parameter homes in
  prepared stack slots instead of materializing a supportable movement plan or
  naming a narrower unsupported boundary.
- The first packet should audit the prepared ABI homes and frame facts before
  deciding whether support is safe.
- If the representative advances to aggregate `va_arg`, non-register parameter
  homes, or another distinct owner, document the handoff instead of expanding
  this plan.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Keep implementation packets small and ABI-fact driven.
- Preserve fail-closed behavior for unsupported or ambiguous byval aggregate
  parameter-home shapes.
- For each code-changing step, run the focused backend build/test proof chosen
  by the supervisor.
- Keep RV64 object-emission, prepared frame-stack call contract, and
  prepared-printer coverage green.
- Rerun `src/20030914-2.c` after the admitted repair or refined diagnostic.

## Steps

### Step 1: Audit Byval Parameter-Home Facts

Goal: identify the exact prepared ABI/home facts available for the
representative byval aggregate parameter home.

Actions:
- Reproduce or load the `src/20030914-2.c` RV64 object-route failure evidence.
- Inspect prepared parameter-home, frame, stack-slot, and aggregate metadata for
  the first byval home that triggers `unsupported_byval_param_home`.
- Identify whether object emission has enough explicit facts to copy or
  address the byval payload safely.
- Record the smallest semantic shape that can be admitted and nearby byval
  shapes that must remain fail-closed.

Completion check:
- `todo.md` names the byval parameter-home shape, the available prepared facts,
  the missing lowering rule or refined unsupported boundary, and the artifacts
  or commands used to identify it.

### Step 2: Implement Byval Home Repair or Refined Boundary

Goal: admit the first safe RV64 object-route byval aggregate parameter-home
class, or replace the coarse rejection with a narrower unsupported diagnostic
when support is not yet semantically justified.

Actions:
- Implement the narrow semantic repair in the responsible RV64 object-route
  parameter-home lowering surface.
- Base the repair only on prepared ABI homes, frame slots, stack slots, and
  aggregate layout facts already published by the prepared contract.
- Keep unsupported stack, aggregate, missing-home, non-default address-space,
  dynamic-layout, and ABI-ambiguous forms fail-closed.
- Keep related RV64 object-route ABI and prepared-printer coverage intact.

Completion check:
- Focused backend tests prove the selected byval aggregate parameter-home
  behavior or refined unsupported boundary and reject adjacent unsupported
  shapes.
- Existing RV64 object-emission, prepared frame-stack call contract, and
  prepared-printer coverage remains green.
- The supervisor-delegated focused build and test command passes.

### Step 3: Rerun `src/20030914-2.c`

Goal: confirm the representative result after the byval parameter-home repair
or refined unsupported boundary.

Actions:
- Rerun `src/20030914-2.c` through the RV64 GCC torture backend progress path
  using the supervisor-provided command.
- Inspect any remaining failure to decide whether it is still the same byval
  parameter-home owner or a distinct next owner.
- If it is the same owner, record the remaining parameter-home gap in `todo.md`
  for another bounded executor packet.

Completion check:
- The representative passes, or `todo.md` documents a distinct next owner with
  artifacts and a handoff recommendation, or `todo.md` documents the still
  in-scope byval parameter-home gap that needs another plan-owner review.

### Step 4: Closure or Handoff Check

Goal: decide whether idea 370 is complete after the byval parameter-home work.

Actions:
- Verify the source idea's byval aggregate parameter-home acceptance criteria
  are satisfied.
- Confirm focused backend coverage for the admitted byval behavior or refined
  unsupported boundary still passes.
- Confirm RV64 object-emission, prepared frame-stack call contract, and
  prepared-printer coverage still passes.
- Ask the plan owner to close idea 370 only if the source idea acceptance
  criteria are satisfied and regression guard can pass.

Completion check:
- Either the supervisor has enough evidence to request closure of idea 370, or
  `todo.md` clearly identifies the remaining blocker and why it is still in
  scope.
