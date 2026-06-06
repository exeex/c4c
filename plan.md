# 114 Prepared Outgoing Stack Argument Area Contract Runbook

Status: Active
Source Idea: ideas/open/114_prepared_outgoing_stack_argument_area_contract.md

## Purpose

Make the total outgoing stack argument reservation/address area an explicit
prepared call-plan contract that shared prealloc owns and target backends
consume.

## Goal

Publish, preserve, print, and consume a call-level outgoing stack argument area
fact without moving target-specific stack adjustment or scratch-base policy into
shared prealloc.

## Core Rule

Shared prealloc owns the target-neutral prepared area fact. Target backends own
physical reservation, restoration, scratch registers, instruction ordering, and
ABI-specific placement.

## Read First

- `ideas/open/114_prepared_outgoing_stack_argument_area_contract.md`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_printer/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`

## Current Targets

- Prepared call-plan surface for call-level outgoing stack area data.
- Prepared lookups and boundary classification that preserve that data.
- Prepared printer visibility for the call-level area.
- Focused AArch64 consumption as proof that target code can use the shared
  prepared area while retaining target-owned decisions.
- Tests covering multi-stack-argument area extent, printer output,
  classification/lookup preservation, and one AArch64 consumer route.

## Non-Goals

- Do not move AArch64's `x16` scratch-base convention into shared data.
- Do not prescribe target instruction ordering from shared prealloc.
- Do not rework source selection, byval aggregate transport, or aggregate
  `va_arg` homes.
- Do not implement x86 or RISC-V aggregate stack argument lowering.
- Do not perform broad call ABI rewrites unrelated to outgoing stack area
  publication.
- Do not weaken existing AArch64 stack-argument expectations.

## Working Model

- The prepared call plan should expose whether an outgoing stack argument area
  exists, its total byte extent, and the alignment/base-address contract needed
  to address all prepared stack destinations for the call.
- Individual stack arguments may still carry per-argument destination offsets,
  but reviewers and target consumers must not need to reconstruct the total
  shared area from only those destinations.
- AArch64 should consume the prepared area as reservation/address authority
  while keeping `x16`, stack pointer adjustment/restoration, and store ordering
  target-owned.

## Execution Rules

- Keep each code slice semantic and bounded; do not add named-fixture shortcuts.
- Prefer build proof plus focused tests after each code-changing step.
- Escalate to broader validation before acceptance if the prepared contract
  touches shared call-boundary behavior beyond the targeted tests.
- Treat expectation downgrades or `unsupported` rewrites as route failure unless
  explicitly approved.
- Keep routine progress in `todo.md`; rewrite this runbook only for a real route
  change.

## Ordered Steps

### Step 1: Inspect Existing Prepared Call Stack Argument Shape

Goal: Identify the current prepared call-plan, lookup, printer, and AArch64
consumer paths for stack argument destinations.

Primary targets:
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_printer/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- Existing tests named in the source idea.

Actions:
- Inspect how per-argument `dest_stack_offset` and size data are represented.
- Locate where a call's total stack destination extent is currently computed,
  inferred, printed, or lost.
- Identify the smallest shared data shape needed for a call-level outgoing
  stack area without encoding target policy.
- Record the first implementation packet and proof command in `todo.md`.

Completion check:
- The executor can name the exact fields/functions to change and the focused
  test bucket that should fail or be extended first.

### Step 2: Add The Shared Prepared Area Contract

Goal: Add or refine a prepared call-plan field/helper that publishes the
call-level outgoing stack argument area.

Primary targets:
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.cpp`

Actions:
- Represent absence of an outgoing stack area distinctly from a zero-sized or
  malformed area.
- Compute the total area from all prepared stack argument destinations, including
  a multi-destination shape where using only the first destination would be
  wrong.
- Preserve target neutrality: no `x16`, concrete stack adjustment, or target
  instruction sequencing in shared data.
- Add focused BIR/prealloc proof for the published total area.

Completion check:
- A focused prealloc test proves one prepared call publishes one total outgoing
  stack area covering all relevant stack destinations.

### Step 3: Preserve The Area Through Lookups And Classification

Goal: Ensure prepared lookup/classification helpers carry and validate the
call-level area as a first-class prepared fact.

Primary targets:
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `tests/backend/bir/backend_prealloc_call_boundary_classification_test.cpp`

Actions:
- Expose lookup accessors or classification data for the outgoing stack area.
- Keep classification from recomputing the total ad hoc from target-side
  argument stores.
- Add or extend tests proving the area survives prepared call-boundary
  classification.

Completion check:
- Classification/lookup proof fails if the call-level area is omitted or
  recomputed only from a narrow per-argument path.

### Step 4: Print The Call-Level Area

Goal: Make the prepared dump show the outgoing stack argument area independently
from per-argument stack offsets.

Primary targets:
- `src/backend/prealloc/prepared_printer/calls.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Actions:
- Add stable prepared-printer output for the call-level area.
- Keep per-argument destination fields visible where they already belong.
- Add a printer test where reviewers can see the total shared area without
  reconstructing it from individual arguments.

Completion check:
- Prepared-printer proof shows a call-level area fact in dumps separate from
  `dest_stack_offset` fields.

### Step 5: Consume The Shared Area In AArch64

Goal: Update AArch64 only enough to use the shared prepared area as the
reservation/addressing authority.

Primary targets:
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

Actions:
- Replace target-side inference of the total outgoing stack area with the shared
  prepared area fact.
- Preserve AArch64-owned `x16` scratch-base use, stack adjustment/restoration,
  and concrete store ordering.
- Add a focused dispatch test proving reservation is driven by the prepared
  area while target-owned instruction details remain unchanged.

Completion check:
- AArch64 dispatch proof fails if the backend ignores the prepared area and
  derives reservation authority solely from per-argument destinations.

### Step 6: Acceptance Validation And Route Review

Goal: Prove the bounded prepared area contract without widening into unrelated
  aggregate call-boundary work.

Actions:
- Run the focused BIR/prealloc, prepared-printer, classification/lookup, and
  AArch64 dispatch tests touched by the route.
- Run a build or broader backend subset if the shared prepared call surface
  changed across multiple consumers.
- Confirm existing c-testsuite AArch64 stack-argument cases remain supported
  without weakening expectations.
- Check the final diff against the source idea's reviewer reject signals.

Completion check:
- The prepared call-plan surface has an explicit, reviewable outgoing stack
  argument area fact; tests cover shared fact, dump visibility, lookup or
  classification preservation, and one target consumer route; no out-of-scope
  aggregate initiative or expectation downgrade landed.
