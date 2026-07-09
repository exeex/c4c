# Aggregate Stack-Home Local-Memory Policy Runbook

Status: Active
Source Idea: ideas/open/633_aggregate_stack_home_local_memory_policy.md

## Purpose

Define the producer and RV64 consumer boundary for local-memory accesses that
flow through aggregate, sret, byval, or pointer stack homes.

## Goal

Move at least one complete-authority aggregate stack-home local-memory family
past its current owner, or reclassify all such rows into precise existing or
new producer-policy owners with current evidence.

## Core Rule

RV64 may consume aggregate stack-home local memory only when explicit prepared
facts identify the source value, destination home, stack slot, selected offset,
access size, and memory-use authority. Do not infer stack-home semantics from
ABI convention, final assembly layout, object shape, source filenames, or row
names.

## Read First

- `ideas/open/633_aggregate_stack_home_local_memory_policy.md`
- `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
- Related open ideas named by the source idea when a row appears to cross
  ownership boundaries:
  - `ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md`
  - `ideas/open/627_pointer_stack_result_call_policy.md`
  - `ideas/open/629_prepared_return_destination_home_authority.md`
- Backend prepared stack-layout and local-memory carrier code under
  `src/backend/prealloc/` and `src/backend/bir/`.
- RV64 local-memory consumers under
  `src/backend/mir/riscv/codegen/object_emission.cpp` and adjacent prepared
  local-memory emission helpers.

## Current Targets

- Residual local-memory rows involving aggregate, sret, byval, or pointer
  stack homes.
- Prepared facts for source value identity, destination home identity, stack
  slot identity, selected offset, access size, selected memory-use authority,
  and any ABI ownership needed before RV64 emission.
- Current diagnostics that distinguish producer-publication gaps from RV64
  consumer gaps.

## Non-Goals

- Outgoing stack argument destination publication owned by idea `624`.
- Pointer stack-result call policy owned by idea `627`.
- Prepared return destination-home authority owned by idea `629`.
- Generic frame-slot local-memory support already closed by idea `614`.
- String/global local-memory policies, F128 width policy, expectations,
  unsupported markers, allowlists, timeouts, runtime/library policy, or
  accounting.
- Broad ABI, return, aggregate-object, or global-object rewrites outside the
  stack-home local-memory boundary.

## Working Model

- Evidence comes first. Refresh the aggregate stack-home residual rows before
  choosing a producer or consumer edit.
- Treat sret, byval, aggregate local copies, and pointer stack homes as related
  but not interchangeable. Only group rows when their prepared authority shape
  is the same.
- Producer-publication gaps should be fixed at the producer boundary. RV64
  consumer admission should be narrow and fact-driven.
- Reclassify rows into idea `624`, `627`, `629`, another existing open idea, or
  a new durable idea when fresh evidence shows they are not aggregate
  stack-home local-memory consumer work.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Do not edit the source idea unless durable intent changes or closure notes
  are required.
- Add focused prepared-layer or RV64 object-emission tests for code-changing
  steps.
- Use `cmake --build --preset default` plus a supervisor-selected focused
  backend subset as the normal proof ladder for code slices.
- Treat expectation rewrites, unsupported-marker changes, testcase-shaped row
  matching, source-file matching, ABI inference, final-layout inference, and
  diagnostic-only changes as route failures.

## Step 1: Refresh Aggregate Stack-Home Evidence

Goal: identify the current residual rows that actually involve aggregate,
sret, byval, or pointer stack-home local-memory traffic.

Actions:
- Re-run focused probes for the aggregate stack-home residual family left
  outside idea `614`.
- Capture current failure owner, selected base shape, source value identity,
  destination home identity, stack slot identity, selected offset, access size,
  memory-use authority, ABI or return role, and current diagnostic.
- Separate in-scope stack-home rows from outgoing stack arguments, pointer
  stack-result calls, return destination homes, generic frame slots,
  string/global rows, F128/16-byte rows, large-offset rows, and unrelated
  runtime/accounting failures.
- Record row evidence and recommended next owner bucket in `todo.md`.

Completion check:
- `todo.md` lists the rows inspected, the current stack-home facts, the first
  missing or rejecting boundary, and a next step that is not named-case-only.

## Step 2: Trace Stack-Home Authority Carriers

Goal: locate the prepared producer and carrier boundary for the in-scope
stack-home family.

Actions:
- Trace construction of the selected local-memory access for the Step 1
  in-scope bucket.
- Identify the carrier fields that already hold, or should hold, source value,
  destination home, stack slot, offset, size, and selected memory-use
  authority.
- Locate the first missing, stale, or ambiguous authority boundary before RV64
  object emission.
- Record fail-closed states for missing home identity, stale source value,
  incomplete offset or size facts, ambiguous aggregate lane, unsupported ABI
  role, and mismatched memory-use authority.

Completion check:
- `todo.md` names the exact producer functions, carrier fields, consumer
  checks, and the smallest code-changing packet that can publish or consume
  stack-home local-memory authority.

## Step 3: Publish Or Verify Prepared Stack-Home Facts

Goal: ensure stack-home local-memory facts are explicit before RV64 lowering
depends on them.

Actions:
- If upstream stack-home authority exists but is not published on the selected
  local-memory carrier, add the narrow producer publication path.
- If publication already exists, add focused coverage proving the facts are
  complete before object emission.
- Preserve source value identity, destination home identity, stack slot,
  selected offset, access size, ABI role, and selected memory-use authority.
- Keep outgoing stack arguments, pointer stack-result calls, and return
  destination homes out of this carrier unless a lifecycle decision moves them
  into this idea.

Completion check:
- Focused backend coverage proves the selected stack-home facts are present in
  the prepared representation, or `todo.md` reclassifies the bucket with
  precise missing upstream authority.

## Step 4: Add Narrow RV64 Stack-Home Consumer Admission

Goal: allow RV64 object emission to consume only explicit aggregate stack-home
local-memory facts for the selected family.

Actions:
- Replace the relevant `unsupported_local_memory_access` rejection with
  validation of stack-home local-memory authority for the selected family.
- Emit only when source value, destination home, stack slot, selected offset,
  access size, ABI role, and memory-use authority match the consumer contract.
- Add fail-closed tests for absent, malformed, ambiguous, stale, mismatched, or
  unsupported stack-home facts.
- Preserve existing frame-slot behavior and keep string/global rows,
  outgoing-argument rows, pointer-result rows, return-only rows, unsupported
  width rows, large-offset rows, and unrelated owners out of this admission
  path.

Completion check:
- Focused positive and negative backend tests pass, and the RV64 path rejects
  stack-home local-memory accesses without explicit prepared authority.

## Step 5: Reclassify Aggregate Stack-Home Rows

Goal: determine whether the source idea is complete, needs another stack-home
policy packet, or should split remaining work into separate initiatives.

Actions:
- Re-run the aggregate stack-home row probes after any Step 3 or Step 4
  changes.
- Classify each remaining failure into stack-home local-memory authority or an
  out-of-scope owner bucket.
- Record whether idea `633` is close-ready or which next stack-home packet is
  justified.

Completion check:
- `todo.md` contains row-by-row classification, proof results, and a clear
  close/split/continue recommendation for the supervisor.

## Step 6: Trace Stack-Home Fallthrough After Authority Gate

Goal: determine why clean byval/sret pointer-value stack-home accesses with
published authority still reach the generic `unsupported_local_memory_access`
diagnostic.

Primary target: `src/pr38969.c`, with `src/pr30185.c` and `src/950628-1.c`
as cross-checks.

Actions:
- Start from the prepared dumps and object-emission diagnostic path for
  `src/pr38969.c`.
- Trace one byval load and one sret store whose prepared address has
  `base=pointer_value`, byval or sret stack-home source objects,
  `base_plus_offset=yes`, complete extent, and
  `range_verdict=proven_in_bounds`.
- Identify the exact branch, predicate, helper, or representation mismatch
  that prevents the Step 4 `prepared_stack_home_local_memory_has_authority(...)`
  consumer from accepting the access.
- Cross-check the same boundary against `src/pr30185.c` and `src/950628-1.c`
  before choosing a code-changing packet.
- Record whether the miss is an RV64 consumer-routing issue, a prepared-address
  shape issue, a stale authority predicate issue, or a separate producer gap.

Completion check:
- `todo.md` names the exact failing boundary and the smallest non-overfit
  Step 7 packet, or reclassifies the clean representatives with evidence that
  idea 633 no longer owns them.

## Step 7: Repair The Narrow Stack-Home Consumer Route

Goal: move the verified clean byval/sret stack-home lane past the generic
local-memory gate without broadening idea 633 into unrelated aggregate,
move-bundle, F128, mixed global, runtime, or ABI work.

Actions:
- If Step 6 proves the prepared facts are complete but the RV64 consumer route
  fails to reach the stack-home authority helper, repair only that routing or
  predicate mismatch.
- If Step 6 proves a prepared-address shape is missing a fact required by
  `prepared_stack_home_local_memory_has_authority(...)`, publish that fact at
  the narrow producer/carrier boundary and keep the RV64 consumer fail-closed.
- Add focused positive coverage for the representative route and negative
  coverage for malformed or out-of-scope shapes that could otherwise be
  admitted by the repair.
- Preserve existing frame-slot, string/global, move-bundle, large-offset,
  F128/16-byte, runtime, and mixed local/global behavior.

Completion check:
- Focused prepared-layer or RV64 object-emission tests pass, and the accepted
  route is justified by explicit stack-home authority rather than row names,
  final layout, ABI convention, or object shape inference.

## Step 8: Reclassify Aggregate Stack-Home Rows After Route Repair

Goal: decide whether idea 633 can close after the follow-up repair or whether
additional stack-home packets remain justified.

Actions:
- Re-run the same aggregate stack-home residual probe used by Step 5.
- Compare the clean byval/sret representatives against the Step 5 baseline.
- Reclassify remaining rows into in-scope stack-home gaps, existing open ideas,
  or new durable residual owners.
- Record whether the source idea is close-ready, needs another numbered
  stack-home step, or should split residual initiatives.

Completion check:
- `todo.md` records the refreshed probe result, representative movement or
  non-movement, residual owner buckets, and a close/continue/split
  recommendation grounded in current evidence.
