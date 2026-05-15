# Prepared Stack-Slot Preserved-Value Extents Runbook

Status: Active
Source Idea: ideas/open/242_prepared_stack_slot_preserved_value_extent.md

## Purpose

Expose explicit prepared/shared size and alignment facts for stack-slot
`PreparedCallPreservedValue` records so later target call lowering can build
complete structured memory-preserve effects from prepared authority alone.

## Goal

Stack-slot preserved values carry stable value identity, slot identity, stack
offset, size, and alignment before any AArch64 consumer attempts to convert
them into memory effects.

## Core Rule

Do not infer stack-slot preserved-value size or alignment inside AArch64
lowering. The extent must be produced by prepared/shared facts and remain
observable before target lowering consumes it.

## Read First

- `ideas/open/242_prepared_stack_slot_preserved_value_extent.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`

## Current Targets

- `prepare::PreparedCallPreservedValue`
- `prepare::build_call_preserved_values`
- prepared dump or call-plan observation output for preserved values
- tests or fixtures that exercise a call-crossing stack-slot preserved value
- AArch64 call-preserve conversion only as a consumer/proof surface, not as the
  owner of extent inference

## Non-Goals

- Do not redesign frame-slot allocation, spill/reload policy, outgoing call
  stack areas, scratch-register selection, or variadic register-save areas.
- Do not change existing register preserved-value carriers except where needed
  to keep their behavior stable after adding stack-slot extent fields.
- Do not downgrade preserved-value or call tests to claim progress.
- Do not add named-case shortcuts for one stack-slot fixture.
- Do not require broad AArch64 call lowering integration unless the prepared
  extent facts are already proven and the integration remains narrow.

## Working Model

`PreparedCallPreservedValue` already identifies live-across-call values and can
route them through callee-saved registers or stack slots. Stack-slot routes
currently retain slot id, stack offset, and spill-slot placement, but not the
storage size and alignment required for a complete structured memory operand.
The implementation should attach extent data at the prepared layer by using
existing frame-slot, value-home, or spill-slot authority, then make missing
extent states explicit instead of allowing target-local reconstruction.

## Execution Rules

- Prefer adding fields and producer logic at the prepared/shared layer over
  adding target-specific fallback logic.
- Preserve existing callee-saved-register preserved-value behavior.
- Keep extent values tied to stable slot identity and prepared stack offset.
- Make prepared observations show the new facts before using them in target
  consumers.
- Each code-changing step needs fresh build proof plus the narrow test subset
  named by the supervisor.
- Escalate to broader validation if the implementation touches shared frame
  allocation, spill/reload policy, or call-plan construction beyond preserved
  value extent plumbing.

## Ordered Steps

### Step 1: Map Existing Prepared Authority

Goal: identify the authoritative source of stack-slot size and alignment for
each current `PreparedCallPreservedValue` stack-slot route.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`

Actions:

- Inspect every branch in `build_call_preserved_values` that sets
  `PreparedCallPreservationRoute::StackSlot`.
- For each branch, identify whether size and alignment should come from a
  `PreparedValueHome`, a frame slot, an assigned stack slot, or another
  prepared record.
- Record any branch where extent authority is absent as an explicit blocker in
  `todo.md`; do not patch it in AArch64.

Completion check:

- The executor can name the exact producer field or missing-authority state for
  every stack-slot preserved-value route before editing extent consumers.

### Step 2: Add Prepared Extent Fields

Goal: make stack-slot preserved-value size and alignment first-class prepared
facts.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`

Actions:

- Add explicit optional or zero-safe size/alignment fields to
  `PreparedCallPreservedValue` for stack-slot routes.
- Populate those fields from prepared authority in each stack-slot producer
  branch identified in Step 1.
- Leave register-route preserved values behaviorally unchanged.
- Make unsupported or missing extent states explicit rather than silently
  fabricating size or alignment.

Completion check:

- Prepared stack-slot preserved values carry stable value id, slot id, stack
  offset, size, alignment, and existing placement data; register preserved
  values continue to carry the same register facts as before.

### Step 3: Expose Extents In Prepared Observations

Goal: make the new extent facts visible before target lowering consumes them.

Primary targets:

- `src/backend/prealloc/prepared_printer.cpp`
- any existing prepared dump or call-plan test fixture selected by the
  supervisor

Actions:

- Extend preserved-value dump formatting to show stack-slot size and alignment
  when present.
- Add or update focused coverage for a call-crossing stack-slot preserved value
  that proves size and alignment are emitted from prepared facts.
- Keep test expectations semantic; do not weaken unrelated call or preserved
  value contracts.

Completion check:

- A prepared observation or call-plan fixture shows stack-slot preserved-value
  size and alignment without relying on AArch64 target lowering.

### Step 4: Prove AArch64 Can Consume Prepared Extents

Goal: demonstrate that AArch64 call lowering no longer needs local inference
to build a complete memory-preserve effect.

Primary targets:

- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`

Actions:

- Inspect the current AArch64 stack-slot preserved-value consumer that returns
  no memory effect when extent data is absent.
- If the prepared fields are sufficient, convert stack-slot preserved values to
  structured memory-preserve effects using only prepared slot id, stack offset,
  size, and alignment.
- If integration is larger than a narrow consumer change, stop after prepared
  proof and record the remaining target integration as follow-up in `todo.md`
  for supervisor routing.

Completion check:

- Either AArch64 consumes prepared stack-slot extent facts without local
  inference, or the prepared extent route is proven and the remaining AArch64
  integration is explicitly deferred without weakening the source idea.

### Step 5: Validation And Reviewer Readiness

Goal: prove the route is capability progress, not testcase-shaped expectation
churn.

Actions:

- Run a fresh build or compile proof after the implementation slice.
- Run the supervisor-selected narrow prepared/call/AArch64 subset.
- Preserve `test_after.log` as the executor proof log unless the supervisor
  delegates another artifact.
- Check for regressions in existing register preserved-value behavior and
  non-stack-slot call/frame facts.

Completion check:

- Build proof and narrow tests pass, the prepared extent facts are observable,
  and no test was weakened to claim progress.

## Reviewer Reject Signals

Reject implementation as route drift if it infers stack-slot preserved-value
size or alignment inside AArch64 lowering from value type, slot order, stack
offset, object name, or frame-slot ordering; weakens preserved-value or call
tests; renames existing preserved-value fields without adding explicit size
and alignment authority; hides spill, storage-extent, or call-preservation
decisions inside target codegen; or broadens into unrelated frame allocation,
outgoing argument areas, scratch-register policy, or variadic save-area work.
