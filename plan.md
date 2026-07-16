# LIR Family-Overloaded Verifier, Dispatch, and Printer Runbook

Status: Active
Source Idea: ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md
Activated after: ideas/closed/845_lir_typed_reference_carriers_collector_migration.md

## Purpose

Replace universal LIR type-family classification at verifier, semantic
dispatch, and printer callsites with explicit family overloads after the
producer and carrier foundations from ideas 838 through 845.

## Goal

Migrate family-sensitive verifier, dispatch, and one-way printer consumers to
nominal overloads without parsing display text or retaining universal
classification as semantic authority.

## Core Rule

Do not rename the universal model behind overload-shaped helpers. Each packet
must select one exact verifier, dispatcher, or printer family surface, prove it
uses native family authority, and keep rendering parity check-only.

## Read First

- `ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md`
- Closed dependency records for ideas 838 through 845, especially the accepted
  carrier and collector migration boundaries.
- Existing LIR type verifier helpers, semantic dispatch code, and printer
  paths that still classify through generic or string-backed type state.
- Nearby frontend/LIR tests that cover valid, malformed, foreign,
  wrong-module, wrong-family, dispatch, and rendering parity behavior.

## Current Scope

- Nominal overloads such as `require_scalar`, `require_vector`,
  `require_aggregate`, `require_signature`, and value-family checks.
- One exact named verifier, dispatch, or printer consumer family per packet.
- Rendering parity only as a one-way check after native semantic authority is
  already selected.

## Non-Goals

- No producer/store construction or new semantic carrier design.
- No universal-model deletion; that belongs to idea 847 after this route's
  gates are accepted.
- No parsing display text, runtime text, diagnostics, or rendered printer
  output as semantic state.
- No Raw-BIR receiver work, 734 receiver repair, or 797 terminal convergence.
- No broad verifier or printer rewrite without a selected family boundary.

## Working Model

Ideas 838 through 845 established the earlier family producers, carriers, and
collector migrations needed before verifier/dispatch/printer consumers can
move off universal classification. This runbook should first identify the
lowest-risk remaining generic family consumer, then migrate bounded surfaces
until every named caller uses a family overload and rendering is check-only.
Idea 847 remains blocked until the deletion gates from this route are accepted.

## Execution Rules

- Treat Step 1 as trace and selection; do not edit implementation code unless
  the selected first consumer is already isolated and explicitly recorded in
  `todo.md`.
- Update `todo.md` before each code-changing packet.
- Prefer a single consumer family per implementation packet.
- Preserve byte-for-byte rendering parity where the printer is involved, but
  reject any route that uses parity to recover semantics.
- Add nearby positive and malformed/foreign/wrong-module/wrong-family coverage
  for verifier overloads and exhaustive dispatch coverage where dispatch is
  changed.
- Run a fresh build plus focused same-feature tests for each packet; broaden
  when shared verifier, dispatch, or printer infrastructure changes.

## Ordered Steps

### Step 1 - Select One Generic Family Consumer

Goal: identify one exact universal-classification consumer that can be
migrated to native family overloads with existing producer/carrier authority.

Actions:
- Inspect remaining generic LIR type-family verifier, semantic dispatch, and
  printer callsites.
- Confirm the selected callsite's semantic facts are already published by the
  dependency chain; if not, classify the missing producer/carrier owner instead
  of repairing it here.
- Record the selected consumer, required overload, expected tests, and
  non-goals in `todo.md`.

Completion check:
- `todo.md` names one exact verifier, dispatch, or printer consumer to migrate,
  or records a lifecycle blocker with evidence that no current 846 consumer is
  ready.

### Step 2 - Add The Bounded Family Overload

Goal: introduce or complete the overload needed by the selected consumer
without preserving universal classification as its semantic implementation.

Actions:
- Add the narrow family-specific verifier, dispatch, or printer overload.
- Use native semantic family authority and module/store ownership where
  required.
- Keep generic helpers only where unselected callers still depend on them.
- Avoid producer, carrier, Raw-BIR, or deletion work outside the selected
  consumer family.

Completion check:
- The selected overload validates the correct family and rejects malformed,
  foreign, wrong-module, and wrong-family inputs without text recovery.

### Step 3 - Migrate The Selected Consumer

Goal: switch the selected verifier, dispatch, or printer consumer to the
bounded overload.

Actions:
- Replace only the selected generic classification call path.
- Preserve one-way rendering parity when the printer is involved.
- Leave unrelated generic callers untouched for later packets.
- Remove a generic helper only if this packet proves it has no remaining
  semantic callers.

Completion check:
- The selected consumer no longer depends on universal/string classification
  for semantics, and no unrelated family route changed.

### Step 4 - Prove The Packet

Goal: prove the selected migration is behavior-preserving for valid inputs and
fail-closed for malformed authority.

Actions:
- Add or update nearby valid and malformed/foreign/wrong-module/wrong-family
  tests for the selected route.
- Add exhaustive dispatch or byte-for-byte rendering parity coverage when the
  selected route touches dispatch or printing.
- Run a fresh build, focused same-feature tests, and `git diff --check`.
- Run broader proof if shared verifier, dispatch, or printer infrastructure
  changed.

Completion check:
- Focused proof passes, broader proof is selected where warranted, and the
  packet does not retain semantic text classification behind overload names.

### Step 5 - Decide Next 846 Packet Or 847 Handoff

Goal: decide whether more verifier/dispatch/printer consumers remain under
846 or whether the accepted gates are sufficient to hand off to idea 847.

Actions:
- Inventory remaining universal classifier, renderer, mutable semantic string,
  and implicit conversion callers.
- Classify each remaining caller as already migrated, still in 846 scope, or
  blocked by another producer/carrier owner.
- If 846 still owns a caller, repair the runbook to the next bounded consumer.
- If all M12/M13 gates are accepted, record the exact deletion-gate handoff for
  idea 847.

Completion check:
- The next executable lifecycle action is explicit: continue 846 with one
  bounded consumer, create/switch to a prerequisite blocker, or activate 847
  only after all 846 gates are accepted.
