# Prepared Outgoing Stack Argument Destination Offsets Runbook

Status: Active
Source Idea: ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md

## Purpose

Publish prepared destination stack offset and size facts for outgoing
stack-slot call arguments so RV64 call consumers do not infer ABI layout.

## Goal

Refresh current outgoing-stack argument diagnostics, identify the producer
surface that owns destination stack authority, and add the narrow prepared
fact publication needed by byval or aggregate stack-copy call arguments.

## Core Rule

Do not infer outgoing destination offsets in RV64 object emission from ABI
index, final assembly layout, source filename, or testcase shape. The repair
must publish explicit prepared destination stack offset and size authority
before consumers use it.

## Read First

- `ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md`
- `review/613_step2_byval_outgoing_stack_slice_review.md`
- `ideas/closed/613_abi_call_result_stack_frame_lowering.md`

## Current Targets

- Prepared call-boundary facts for outgoing stack-slot call arguments.
- Byval aggregate or stack-copy payloads transported to the outgoing stack
  area.
- Destination stack offset, destination size, payload identity, source
  identity, argument identity, and callsite association facts.
- Fail-closed RV64 guards for missing or conflicting prepared outgoing stack
  destination authority.

## Non-Goals

- RV64 object-emission lowering that guesses completed facts.
- Variadic or library call policy.
- Runtime mismatch triage.
- Local/global producer repair, stack-frame consumer repair, expectation
  changes, unsupported marker changes, allowlists, timeouts, or accounting.
- Named-case handling for `src/20000808-1.c`, the `931004-*` family,
  `src/931031-1.c`, `src/950607-2.c`, `src/pr69447.c`, or any other torture
  row.

## Working Model

- Treat Step 1 as evidence refresh and owner classification before any code
  change.
- The desired authority is producer-side prepared call-boundary data:
  destination stack offset and destination size for outgoing stack-slot
  arguments.
- RV64 consumers may validate and require the prepared facts, but they must not
  manufacture destination offsets locally.
- A single representative row is not enough; use adjacent aggregate/outgoing
  stack argument rows as breadth evidence and guard rows.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Do not edit `ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md`
  unless durable source intent changes.
- Preserve unsupported diagnostics when outgoing stack area, destination
  offset, destination size, or payload coverage is incomplete.
- Do not downgrade supported-path expectations or weaken test contracts.
- Do not claim progress through expectation, unsupported-marker, allowlist,
  timeout, runtime, or accounting rewrites.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact.
- Escalate to supervisor/reviewer if the available route is only a testcase
  shortcut, an RV64 inference fallback, or a broad ABI rewrite.

## Ordered Steps

### Step 1: Refresh outgoing-stack argument evidence

Goal: Replace inherited idea-613 residual notes with current diagnostics.

Concrete actions:
- Run the supervisor-delegated diagnostic refresh for outgoing stack-slot,
  byval, and aggregate stack-copy call argument rows.
- Include `src/20000808-1.c` plus nearby aggregate/outgoing-stack transport
  rows such as the `931004-*` family, `src/931031-1.c`, `src/950607-2.c`, and
  `src/pr69447.c` when present in the current corpus.
- Capture row identifiers, callsite facts, move or binding facts, payload
  identity, outgoing stack-area facts, diagnostic text, and first owner.
- Separate producer-authority rows from RV64-consumer, ABI-policy,
  local/global, stack-frame, runtime, variadic/library, and unsupported
  semantic rows.

Completion check:
- `todo.md` records the refresh command, proof artifact, row list, first-owner
  buckets, and negative guard rows.
- No implementation files are changed in this step.

### Step 2: Locate prepared destination authority production

Goal: Identify the prepared call-boundary producer surface that should publish
destination stack offsets and sizes.

Concrete actions:
- Inspect where ordinary call argument preparation creates outgoing stack-slot
  moves, bindings, byval stack-copy payloads, and outgoing stack-area metadata.
- Trace whether destination offset and size are already known in frame or ABI
  planning before RV64 object emission.
- Identify the smallest producer-side data structure or fact publication point
  that can carry destination offset, destination size, payload identity,
  argument identity, and callsite association.
- Record any rows that lack enough upstream authority and must remain
  fail-closed or move to a more precise owner.

Completion check:
- `todo.md` names the concrete producer target, the facts to publish, and the
  rows expected to prove the route.
- The route does not require RV64 to infer destination offsets from ABI index
  or final assembly shape.

### Step 3: Publish prepared outgoing destination facts

Goal: Add the narrow producer-side authority required by outgoing stack-slot
call arguments.

Primary target:
- The prepared call-boundary producer surface identified in Step 2.

Concrete actions:
- Extend the prepared fact model to carry explicit destination stack offset
  and destination size for complete outgoing stack-slot call arguments.
- Populate those fields only when outgoing stack area, payload coverage, and
  destination size are proven.
- Preserve source value, payload identity, argument identity, and callsite
  association for downstream validation.
- Add focused producer-level or diagnostic tests that prove the facts are
  present before RV64 object emission.
- Keep incomplete, ambiguous, undersized, or conflicting destination facts
  rejected with precise diagnostics.

Completion check:
- At least one ordinary same-module byval or outgoing-stack aggregate row
  exposes explicit prepared destination offset and size facts.
- Missing-authority and conflicting-authority negative cases remain
  fail-closed.
- Fresh build or compile proof is recorded in `test_after.log`.

### Step 4: Wire consumer guards without inference

Goal: Make downstream RV64 call consumers require the prepared facts without
becoming the authority producer.

Concrete actions:
- Inspect existing RV64 call ABI diagnostics for outgoing stack-slot
  arguments and byval stack-copy payloads.
- Consume the prepared destination offset and size facts when present.
- Keep diagnostics fail-closed when the outgoing stack area, destination
  offset, destination size, or payload coverage is missing or contradictory.
- Do not derive offsets from ABI argument index, source filename, final object
  layout, or final assembly shape.

Completion check:
- `src/20000808-1.c` either moves past the missing prepared destination
  authority blocker or is reclassified to a more precise current owner.
- Nearby aggregate/outgoing-stack guard rows either progress for the same
  semantic reason or remain rejected with precise owner diagnostics.
- Fresh build or compile proof is recorded in `test_after.log`.

### Step 5: Prove breadth and close-readiness

Goal: Validate that the route repaired prepared outgoing destination
authority, not one named row.

Concrete actions:
- Re-run the outgoing-stack argument diagnostic subset from Step 1.
- Compare first-owner buckets before and after the producer/consumer changes.
- Include negative guards for incomplete outgoing stack area, missing
  destination offset, undersized destination size, and conflicting destination
  facts.
- Run the supervisor-selected build/test subset for the touched surfaces.
- Record remaining residuals by owner and recommend whether this source idea
  is complete, blocked, or needs a narrower follow-up split.

Completion check:
- Proof shows explicit prepared destination offset and size facts for a shared
  outgoing-stack argument family, or documents a precise no-route blocker.
- No expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting-only change is used as progress.
- `todo.md` contains proof commands, results, residual owner buckets, and
  follow-up notes for supervisor review.
