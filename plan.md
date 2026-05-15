# I128 Deferred Helper Family Authority Runbook

Status: Active
Source Idea: ideas/open/250_i128_deferred_helper_family_authority.md
Activated from: ideas/open/250_i128_deferred_helper_family_authority.md

## Purpose

Define the prepared/shared authority needed for i128 helper families that were
explicitly deferred from the supported AArch64 i128 pair-lowering route:
float/i128 conversion helpers and helper families that return through memory.

## Goal

Expose source-operation, helper, lane/result, ABI, marshaling,
clobber/resource, live-preservation, and memory-return ownership facts so a
later AArch64 consumer can select and print supported deferred i128 helper
boundaries without target-local helper synthesis or fixed-register assumptions.

## Core Rule

This runbook prepares helper-family authority only. Do not reopen direct-result
div/rem helper-call printing, hard-code helper ABI registers, lower helper
operations as scalar i64 shortcuts, or move helper selection/memory-return
ownership into AArch64 target lowering.

## Read First

- `ideas/open/250_i128_deferred_helper_family_authority.md`
- `ideas/closed/236_aarch64_i128_pair_lowering.md`
- `ideas/closed/248_prepared_i128_runtime_helper_authority.md`
- `ideas/closed/249_prepared_i128_helper_marshaling_abi_binding.md`
- prepared i128 carrier, runtime-helper, ABI binding, marshaling,
  live-preservation, and selected-call ownership facts
- focused backend tests under `tests/backend/mir/`

## Current Targets

- Source-operation mapping for supported i128 float-conversion helpers.
- Helper kind and callee identity for deferred helper families.
- Argument/result lane ownership for conversion helpers.
- Memory-return ownership facts: destination identity, storage extent,
  alignment, slot, offset, and live-preservation requirements.
- ABI/register-bank binding, marshal/unmarshal, clobber/resource, and
  selected-call ownership facts for deferred helper families.
- Fail-closed diagnostics for unsupported or incomplete helper authority.

## Non-Goals

- Do not reopen direct-result div/rem helper-call printing already covered by
  idea 236 unless inspection finds a concrete regression or missing shared
  fact.
- Do not implement AArch64 selected records or terminal printer output in this
  prerequisite unless a later active plan explicitly owns consumption.
- Do not lower helper-required operations as scalar i64 shortcuts.
- Do not hard-code helper ABI registers, fixed low/high lane pairs, register
  adjacency, rendered names, or helper callee strings in AArch64 target
  lowering.
- Do not broaden into binary128 soft-float, scalar FP, atomics, intrinsics,
  inline asm, generic call lowering, callee-save placement, or preserved-value
  extent work.

## Working Model

Ideas 248 and 249 established the authority pipeline for direct-result i128
div/rem helpers. This runbook applies that model only to the helper families
left out of idea 236: float/i128 conversions and memory-return helper shapes.
If a helper family cannot expose complete structured facts, keep it
fail-closed and record the exact missing prepared/shared authority instead of
patching the gap in target lowering.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Start with inspection; do not assume all deferred families share the
  direct-result div/rem shape.
- Add prepared/shared carriers before any selected AArch64 consumer attempts
  to lower or print deferred helper boundaries.
- Preserve existing supported div/rem helper-call behavior.
- Preserve fail-closed diagnostics for incomplete source-operation mapping,
  helper identity, memory-return ownership, ABI bindings, marshaling,
  clobber/resource, and live-preservation state.
- Treat hard-coded ABI registers, rendered-name recovery, scalar-i64
  shortcuts, expectation rewrites, and named-case helper matching as route
  drift.
- For code-changing packets, prove with a build plus the supervisor-chosen
  focused prepared/i128 backend subset. Escalate to broader backend validation
  after shared ABI, call, memory-return, or printer-visible facts change.

## Ordered Steps

### Step 1: Inspect Deferred Helper Family Gaps

Goal: identify which i128 float-conversion and memory-return helper families
can be prepared now, and which facts are missing.

Primary targets:

- i128 float-conversion BIR/source operation shapes
- existing `PreparedI128RuntimeHelper` and carrier facts
- memory-return and destination ownership surfaces
- ABI binding, marshaling, clobber/resource, live-preservation, and
  selected-call ownership facts

Actions:

- Trace representative float/i128 conversion and memory-return helper cases
  from BIR through prepared state.
- Identify required source-operation, helper kind, callee, argument/result
  lane, memory destination, storage extent, ABI/register-bank, marshaling,
  clobber/resource, and live-preservation facts.
- Compare those requirements against the direct-result div/rem helper model.
- Record the first implementation packet target and focused proof subset in
  `todo.md`.

Completion check:

- `todo.md` records whether execution can proceed to source-operation/helper
  mapping or must split a narrower prerequisite with exact missing facts.

### Step 2: Add Source-Operation And Helper Mapping

Goal: expose helper kind and callee authority for supported deferred i128
helper families.

Actions:

- Define source-operation mapping for supported i128 float-conversion helpers.
- Define helper kind and callee identity for each supported deferred family.
- Preserve source/result type facts and operand value identity.
- Keep unsupported helper families fail-closed.
- Add focused coverage proving mapping is structural, not fixture-shaped.

Completion check:

- Supported deferred helper families expose source-operation mapping, helper
  kind, and callee facts, while unsupported families diagnose explicitly.

### Step 3: Add Lane, Result, And Memory-Return Ownership

Goal: preserve result ownership for both direct-lane and memory-return helper
shapes.

Actions:

- Add argument and result lane ownership facts for supported conversion helper
  shapes.
- Add memory-return ownership facts where required: destination identity,
  storage extent, alignment, slot, offset, and ownership.
- Preserve value identity, lane order, carrier kind, and storage provenance.
- Add diagnostics and focused coverage for incomplete memory-return ownership.

Completion check:

- Deferred helpers expose complete direct-result or memory-return ownership
  facts, or fail closed with exact diagnostics.

### Step 4: Add ABI Binding And Marshaling Authority

Goal: expose structured ABI/register-bank bindings and marshal/unmarshal facts
for supported deferred helper families.

Actions:

- Add ABI argument and result binding facts for supported helper shapes.
- Add marshal/unmarshal facts between prepared carriers, memory-return
  destinations, and helper ABI bindings.
- Preserve register bank, lane width, move direction, and helper ownership.
- Keep missing bindings and wrong carrier shapes fail-closed.
- Add focused coverage for complete and incomplete binding states.

Completion check:

- Later selected consumers can see every required helper ABI binding and
  marshaling move as structured facts.

### Step 5: Add Clobber, Resource, Live-Preservation, And Selected-Call Facts

Goal: make helper call policy complete enough for later selected-record and
printer consumers.

Actions:

- Connect helper clobber/resource policy to each supported deferred family.
- Evaluate live-preservation requirements at helper program points.
- Preserve selected-call ownership only when callee identity, resources,
  clobbers, ABI bindings, marshaling/unmarshaling, memory-return ownership, and
  live preservation are complete.
- Add focused coverage for complete and incomplete helper policy facts.

Completion check:

- Supported deferred helper boundaries expose complete selected-call ownership
  or fail closed with explicit missing-authority diagnostics.

### Step 6: Validate And Summarize

Goal: prove the prepared/shared deferred helper authority and preserve any
remaining unsupported families.

Actions:

- Run the supervisor-chosen build and focused prepared/i128 backend subset.
- Escalate to broader backend validation if shared ABI, call, memory-return,
  or printer-visible facts changed beyond one carrier.
- Summarize supported deferred helper families and remaining unsupported shapes
  in `todo.md`.
- Ask the supervisor whether to close this prerequisite or activate a later
  selected AArch64 consumer route.

Completion check:

- Supported deferred i128 helper families expose complete structured authority,
  existing div/rem helper behavior remains stable, and unsupported shapes
  remain explicit non-overfit blockers.
