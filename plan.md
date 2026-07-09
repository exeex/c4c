# Prepared Stack-Slot Preservation Source Publication Runbook

Status: Active
Source Idea: ideas/open/625_prepared_stack_slot_preservation_source_publication.md

## Purpose

Publish concrete prepared source endpoints for stack-slot preserves that keep
call-live values across ordinary same-module calls, so RV64 call ABI consumers
do not infer preserve sources from ABI parameter position, storage summaries,
or final assembly shape.

## Goal

Refresh current preserve diagnostics, identify the prepared call-boundary
producer surface that owns preservation source authority, and publish explicit
source endpoint facts for stack-slot preserves before RV64 object emission.

## Core Rule

Do not infer stack-slot preserve source registers inside RV64 from ABI
parameter position, function storage summaries, source filename, testcase
shape, or final assembly layout. The repair must publish explicit prepared
source endpoint authority before consumers use it.

## Read First

- `ideas/open/625_prepared_stack_slot_preservation_source_publication.md`
- `ideas/closed/613_abi_call_result_stack_frame_lowering.md`

## Current Targets

- Ordinary same-module call-boundary preservation facts.
- Stack-slot preserves for values that must survive a call.
- Concrete source register or source-location endpoint, value identity,
  destination stack slot, preserve size/width, and callsite association facts.
- RV64 fail-closed guards for missing, ambiguous, stale, or summary-only
  preserve source authority.
- Current diagnostics for `src/20020529-1.c` and nearby ordinary-call preserve
  rows when present in the corpus.

## Non-Goals

- RV64 object-emission lowering that guesses completed preserve facts.
- Inferring preserve sources from ABI parameter position, function storage
  summaries, testcase names, or final assembly shape.
- Outgoing stack argument destination offsets covered by idea 624.
- Variadic or library call policy.
- Runtime mismatch triage.
- Local/global producer repairs, stack-frame consumer repairs, expectation
  changes, unsupported marker changes, allowlists, timeouts, or accounting.

## Working Model

- Treat Step 1 as evidence refresh and owner classification before any code
  change.
- The desired authority is producer-side prepared call-boundary data: concrete
  source endpoint facts for stack-slot preserves.
- RV64 consumers may validate and require prepared source facts, but they must
  not manufacture source registers from unrelated storage state.
- `src/20020529-1.c` is a representative row, not an implementation target;
  use adjacent preserve rows as breadth evidence and guard rows when available.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Do not edit `ideas/open/625_prepared_stack_slot_preservation_source_publication.md`
  unless durable source intent changes.
- Preserve unsupported diagnostics when source endpoint authority is missing,
  ambiguous, stale, or only derivable from unrelated storage summaries.
- Do not downgrade supported-path expectations or weaken test contracts.
- Do not claim progress through expectation, unsupported-marker, allowlist,
  timeout, runtime, or accounting rewrites.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact.
- Escalate to supervisor/reviewer if the available route is only a testcase
  shortcut, an RV64 inference fallback, or a broad ABI rewrite.

## Ordered Steps

### Step 1: Refresh stack-slot preserve evidence

Goal: Replace inherited idea-613 notes with current diagnostics for ordinary
same-module stack-slot preserves.

Concrete actions:
- Run the supervisor-delegated diagnostic refresh for ordinary call-boundary
  preserve rows that carry stack-slot destinations.
- Include `src/20020529-1.c` and adjacent ordinary-call preserve rows when
  present in the current corpus.
- Capture row identifiers, callsite facts, preservation facts, value identity,
  destination stack slot, preserve size/width, source endpoint text, diagnostic
  text, and first owner.
- Separate prepared producer-authority rows from RV64-consumer, ABI-policy,
  local/global, stack-frame, runtime, variadic/library, and unsupported
  semantic rows.

Completion check:
- `todo.md` records the refresh command, proof artifact, row list, first-owner
  buckets, and negative guard rows.
- No implementation files are changed in this step.

### Step 2: Locate prepared preserve source authority production

Goal: Identify the prepared call-boundary producer surface that should publish
concrete preserve source endpoints.

Concrete actions:
- Inspect where ordinary call preparation creates stack-slot preserves for
  call-live values.
- Trace whether the concrete source register or source location is already
  known before RV64 object emission.
- Identify the smallest producer-side data structure or fact publication point
  that can carry source endpoint, value identity, destination stack slot,
  size/width, and callsite association.
- Record any rows that lack enough upstream authority and must remain
  fail-closed or move to a more precise owner.

Completion check:
- `todo.md` names the concrete producer target, the facts to publish, and the
  rows expected to prove the route.
- The route does not require RV64 to infer preserve sources from ABI parameter
  position, function storage summaries, or final assembly shape.

### Step 3: Publish prepared preserve source facts

Goal: Add the narrow producer-side authority required by stack-slot preserves.

Primary target:
- The prepared call-boundary producer surface identified in Step 2.

Concrete actions:
- Extend the prepared fact model to carry explicit concrete source endpoint
  facts for complete stack-slot preserves.
- Populate those fields only when the source endpoint, destination stack slot,
  preserve size/width, value identity, and callsite association are proven.
- Preserve diagnostics for incomplete, ambiguous, stale, or summary-only
  source authority.
- Add focused producer-level or diagnostic tests that prove the facts are
  present before RV64 object emission.

Completion check:
- At least one ordinary same-module stack-slot preserve row exposes an explicit
  concrete prepared source endpoint.
- Missing-source, ambiguous-source, stale-source, and summary-only negative
  cases remain fail-closed.
- Fresh build or compile proof is recorded in `test_after.log`.

### Step 4: Wire RV64 preserve guards without inference

Goal: Make downstream RV64 call ABI consumers require prepared preserve source
facts without becoming the authority producer.

Concrete actions:
- Inspect existing RV64 call ABI diagnostics for stack-slot preserves.
- Consume the prepared source endpoint facts when present.
- Keep diagnostics fail-closed when the source endpoint, destination stack
  slot, size/width, value identity, or callsite association is missing or
  contradictory.
- Do not derive preserve sources from ABI parameter position, storage
  summaries, source filename, final object layout, or final assembly shape.

Completion check:
- `src/20020529-1.c` either moves past the missing preserve-source blocker or
  is reclassified to a more precise current owner.
- Nearby ordinary-call preserve guard rows either progress for the same
  semantic reason or remain rejected with precise owner diagnostics.
- Fresh build or compile proof is recorded in `test_after.log`.

### Step 5: Prove breadth and close-readiness

Goal: Validate that the route repaired prepared preserve source authority, not
one named row.

Concrete actions:
- Re-run the ordinary-call stack-slot preserve diagnostic subset from Step 1.
- Compare first-owner buckets before and after the producer/consumer changes.
- Include negative guards for missing source endpoint, ambiguous source,
  stale source, and summary-only source authority.
- Run the supervisor-selected build/test subset for the touched surfaces.
- Record remaining residuals by owner and recommend whether this source idea
  is complete, blocked, or needs a narrower follow-up split.

Completion check:
- Proof shows explicit prepared source endpoint facts for a shared ordinary
  stack-slot preserve family, or documents a precise no-route blocker.
- No expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting-only change is used as progress.
- `todo.md` contains proof commands, results, residual owner buckets, and
  follow-up notes for supervisor review.
