# Prepared Local Aggregate Slot Layout Facts Runbook

Status: Active
Source Idea: ideas/open/405_prepared_local_aggregate_slot_layout_facts.md

## Purpose

Repair the prepared producer contract for local aggregate and union storage
whose published frame-slot facts are too narrow for the local memory accesses
that RV64 object emission consumes.

## Goal

Make `tests/c/external/gcc_torture/src/20020225-2.c` stop failing first on a
contradictory one-byte prepared union slot for 8-byte or 4-byte local memory
accesses, without teaching RV64 object emission to infer source layout.

## Core Rule

Fix or split producer-side prepared facts. RV64 object emission must consume
explicit coherent slot size, offset, alignment, alias, and range facts rather
than fabricating aggregate or union layout facts from instruction shape.

## Read First

- `ideas/open/405_prepared_local_aggregate_slot_layout_facts.md`
- `ideas/closed/400_rv64_object_route_local_memory_addressing_edges.md` for
  the closed target-emission boundary that exposed this producer defect
- `tests/c/external/gcc_torture/src/20020225-2.c`
- Current prepared BIR / object-route diagnostics for the representative case

## Current Targets

- Representative: `tests/c/external/gcc_torture/src/20020225-2.c`
- Prepared producer surfaces that publish local frame-slot metadata for
  aggregate or union storage
- Consumer diagnostics that reject local memory access width/range mismatches
  against prepared frame-slot facts

## Non-Goals

- Do not add RV64 emitter recovery for C union layout, aggregate aliasing, or
  source-level storage extent.
- Do not reopen scalar or floating-cast lowering already handled by earlier
  ideas.
- Do not absorb broad stack-frame or parameter-home admission work owned by
  `ideas/open/398_rv64_object_route_stack_frame_and_param_home_edges.md`.
- Do not mark tests unsupported, weaken allowlists, or claim progress from
  expectation rewrites.

## Working Model

The target-side local-memory route is allowed to reject contradictory prepared
facts. This plan assumes the remaining `20020225-2.c` failure is caused by
producer publication of local aggregate or union slot metadata that does not
cover the byte range later used by prepared local memory accesses.

If inspection proves the prepared facts are coherent and the target consumer is
wrong, record that in `todo.md` and ask the supervisor for a route decision
before changing RV64 object emission. If inspection finds a distinct producer
family outside local aggregate/union slot layout, split it into a new source
idea instead of widening this plan.

## Execution Rules

- Keep each code slice tied to one observed producer/consumer fact boundary.
- Prefer source-independent facts and semantic local-storage ownership over
  testcase-shaped matching on `20020225-2.c`.
- Use prepared dumps to prove fact coherence before relying on object-route
  success.
- For code changes, use the supervisor-selected proof command. At minimum,
  prove the representative and one same-family nearby case when available.
- Update `todo.md` with current packet state and proof results; do not edit the
  source idea unless the supervisor delegates lifecycle changes.

## Step 1: Classify Contradictory Local Aggregate Slot Facts

Goal: identify the exact prepared slot facts and local memory accesses that
contradict each other in the representative.

Actions:

- Reproduce the current `20020225-2.c` first failure through the RV64 backend
  route with diagnostics and prepared dumps enabled.
- Record the selected frame slot's published size, offset, alignment, alias or
  overlay facts, and valid byte range.
- Record each rejected local memory access width and offset that exceeds or
  contradicts the selected slot range.
- Search for at least one nearby same-family aggregate or union slot case in
  the current gcc_torture backend artifacts or a supervisor-selected subset.
- Decide whether the defect is missing aggregate/union extent publication,
  incorrect field-slot selection, incorrect union overlay modeling, or a
  different producer-side fact family.

Completion check:

- `todo.md` names the concrete producer fact family, the representative
  mismatch, and the narrow next code target.
- If the representative is not actually a producer-side slot fact defect, the
  route is stopped for supervisor review instead of patched opportunistically.

## Step 2: Repair Producer Slot Layout Publication

Goal: publish coherent prepared local aggregate or union slot facts for the
classified family.

Actions:

- Locate the producer path that creates local frame-slot metadata for the
  classified aggregate or union storage.
- Preserve member offsets, overlay semantics, alignment, and byte range
  ownership while widening or redirecting only the facts that are semantically
  wrong.
- Keep byte-field accesses distinct from wider whole-object, member, or
  union-overlay accesses.
- Add or update focused coverage only where the repo already has a matching
  low-level test surface for prepared facts or local memory metadata.

Completion check:

- Prepared dumps for the representative show internally consistent slot size,
  offset, alignment, and range facts for the repaired access family.
- The repair does not fabricate facts in RV64 object emission.

## Step 3: Prove RV64 Object Consumption

Goal: show RV64 object emission consumes the repaired prepared facts and
advances the representative to object emission success or a distinct later
boundary.

Actions:

- Run the supervisor-selected narrow proof for `20020225-2.c`.
- Include at least one same-family aggregate or union slot case when available.
- Check that any remaining failure is not the same contradictory one-byte slot
  fact behind a renamed diagnostic.
- Inspect runtime behavior when the case links and runs through qemu.

Completion check:

- `20020225-2.c` no longer fails first because an 8-byte or 4-byte local memory
  access is matched to a one-byte prepared union slot.
- Proof evidence is recorded in `todo.md` with the exact command and result.

## Step 4: Refresh Scope And Split Residuals

Goal: keep this plan limited to local aggregate/union slot layout facts and
route distinct producer or backend families to their own owners.

Actions:

- Refresh the narrow bucket or representative subset after the repair.
- Classify remaining local aggregate/union slot failures by concrete prepared
  fact family.
- Create or request a separate idea for non-duplicate producer defects, broad
  stack-frame/parameter-home admission, or target-emission bugs outside this
  source idea.

Completion check:

- The local aggregate/union slot layout family is either repaired for the
  representative and nearby cases, or the remaining blockers are documented as
  distinct follow-up ideas.
- No unsupported downgrades, allowlist filtering, or filename-specific fixes
  are used as acceptance evidence.
