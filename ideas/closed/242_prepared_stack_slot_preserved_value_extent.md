# Prepared Stack-Slot Preserved-Value Extents

Status: Closed
Created: 2026-05-14
Closed: 2026-05-15
Parent Context: ideas/open/231_aarch64_call_frame_machine_nodes.md

## Closure Summary

Prepared stack-slot preserved values now carry explicit size and alignment
facts from prepared/shared authority, prepared observations expose those facts,
and AArch64 consumes complete prepared records into structured memory-preserve
effects without target-local extent inference. Missing prepared extents fail
closed.

## Intent

Add explicit prepared/shared size and alignment facts for stack-slot
`PreparedCallPreservedValue` records so target call lowering can expose
structured memory-preserve effects without deriving storage extent locally.

## Why This Exists

The active AArch64 call/frame route discovered that stack-slot preserved values
carry value identity, slot id, stack offset, and spill-slot placement, but do
not carry the preserved storage size and alignment needed to build a complete
structured memory operand.

AArch64 lowering can keep those records as raw call provenance, but it cannot
turn them into structured `preserves` memory effects without reconstructing
call-preservation or frame-slot storage authority in target codegen.

## Scope

- Define a prepared/shared carrier for stack-slot preserved-value size and
  alignment.
- Tie the carrier to the existing `PreparedCallPreservedValue` stack-slot
  route by stable slot identity and stack offset.
- Preserve value identity, slot id, spill-slot placement, size, alignment, and
  fixed stack location as structured facts.
- Make the extent observable in prepared dumps or call-plan observations before
  any target lowering consumes it.
- Prove that AArch64 call lowering can later produce a complete structured
  memory-preserve effect from prepared facts alone.

## Out Of Scope

- Do not infer stack-slot preserved-value size or alignment inside AArch64
  lowering.
- Do not redesign general frame-slot allocation or outgoing call stack areas.
- Do not change callee-save slot placement beyond what is already tracked by
  `ideas/open/241_prepared_callee_save_slot_placement.md`.
- Do not broaden into full spill/reload policy, scratch-register selection, or
  variadic register-save areas.
- Do not require AArch64 target lowering changes in this idea unless a later
  active plan explicitly owns integration.

## Acceptance Criteria

- Stack-slot `PreparedCallPreservedValue` facts expose enough prepared extent
  data for a target to build a complete memory operand: stable identity, stack
  offset, size, and alignment.
- Existing register preserved-value carriers remain unchanged and continue to
  support structured register preserve effects.
- Existing prepared call/frame facts for non-stack-slot cases remain stable.
- Missing or unsupported stack-slot extent states fail explicitly instead of
  being patched by target-local inference.

## Reviewer Reject Signals

Reject the route if it adds AArch64-side inference from value type, slot order,
stack offset, object name, or frame-slot ordering; if it weakens preserved-value
or call tests to claim progress; if it renames existing preserved-value fields
without adding explicit size and alignment authority; if it hides spill,
storage-extent, or call-preservation decisions inside target codegen; or if it
broadens into unrelated frame allocation, outgoing argument areas, scratch
register policy, or variadic save-area work.
