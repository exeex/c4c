# Pointer Stack-Result Call Policy Runbook

Status: Active
Source Idea: ideas/open/627_pointer_stack_result_call_policy.md

## Purpose

Activate idea 627 as an execution runbook for pointer-valued ordinary
same-module call results whose destination is a stack slot or aggregate home.

Goal: publish and consume explicit pointer result destination/home authority so
RV64 lowering does not infer pointer stack-result behavior from source filenames,
final assembly shape, or ad hoc pointer treatment.

## Core Rule

Do not make named-case fixes. Every accepted change must improve the semantic
prepared call-result authority or fail-closed RV64 consumption for a reusable
pointer stack-result family.

## Read First

- `ideas/open/627_pointer_stack_result_call_policy.md`
- Representative rows named there:
  - `src/20030715-1.c`
  - `src/20011113-1.c`
  - `src/20041218-1.c`
  - `src/pr20601-1.c`
  - `src/pr34176.c`
  - `src/pr58209.c`

## Current Targets / Scope

- Prepared call-result facts for pointer-valued ordinary same-module calls.
- Pointer result destination policy when the result home is a stack slot or
  aggregate home.
- RV64 consumer admission and diagnostics for explicit pointer result
  destination/home facts.
- Negative proof that scalar-only, outgoing aggregate stack arguments, FPR,
  variadic, library, runtime, local/global, and missing-authority rows remain
  outside this idea.

## Non-Goals

- Do not revisit ordinary scalar GPR call/result transport from idea 613.
- Do not fold in outgoing stack aggregate argument offsets, stack-slot preserve
  source publication, dynamic-frame callee-saved placement, FPR policy,
  variadic/library policy, runtime mismatch triage, local/global producer
  repair, expectation changes, unsupported marker changes, allowlists,
  timeouts, or accounting.
- Do not derive pointer result homes from final assembly layout, ABI register
  names alone, aggregate shape, or source filenames.

## Working Model

The backend needs a prepared-facts handoff that answers:

- which ordinary callsite produced the pointer result
- which value identity is being stored or materialized
- which stack slot or aggregate home owns the destination
- which width and pointer representation apply
- whether the RV64 consumer can admit the row or must reject it with a precise
  missing-authority diagnostic

Existing scalar or aggregate routes may be useful references, but this runbook
must keep pointer stack-result policy separate from unrelated ABI buckets.

## Execution Rules

- Start each packet from prepared facts and diagnostics, not final assembly
  expectations.
- Prefer producer authority over consumer inference.
- Add narrow tests only when they prove a semantic authority or fail-closed
  rule, not a source-file special case.
- Keep `todo.md` as the live packet scratchpad; update `plan.md` only for real
  route changes.
- For code-changing steps, run at least:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  unless the supervisor delegates a different exact proof command.

## Steps

### Step 1: Refresh Pointer Stack-Result Evidence

Goal: identify the current shared blocker and separate in-scope pointer
stack-result rows from adjacent buckets.

Primary targets:
- prepared dumps and diagnostics for the representative rows
- existing backend call/result preparation and RV64 object-route admission logs

Actions:
- Re-run or inspect current prepared/backend evidence for each representative
  source.
- Record which rows are ordinary same-module pointer call results with a stack
  slot or aggregate result home.
- Record current prepared facts for callsite identity, result value identity,
  destination home, width, and consumer rejection reason.
- Classify rows that belong to scalar-only, outgoing aggregate argument, FPR,
  variadic/library, runtime, local/global, or missing-authority buckets as
  negative proof rather than implementation targets.

Completion check:
- `todo.md` names the in-scope pointer stack-result family, the missing or
  malformed authority, and the first producer or consumer surface to inspect.

### Step 2: Locate Prepared Producer Authority

Goal: find where pointer result destination/home authority should be produced
without adding RV64 consumer inference.

Primary targets:
- call-result preparation code
- prepared frame/result data carriers
- existing scalar result and aggregate destination-home helpers

Actions:
- Trace the prepared call-result path for an in-scope representative row from
  source-level call result to prepared backend facts.
- Identify whether existing carrier fields can represent pointer result home,
  stack slot or aggregate home, width, value identity, and callsite identity.
- If existing carriers are sufficient, record the producer guard currently
  suppressing publication.
- If carriers are insufficient, define the minimal prepared fact extension
  needed before RV64 admission can be widened.

Completion check:
- `todo.md` records the exact producer function(s), carrier fields or missing
  fields, and the fail-closed condition that should gate Step 3.

### Step 3: Publish Pointer Result Destination/Home Facts

Goal: make prepared facts carry explicit pointer stack-result authority for the
in-scope ordinary call-result family.

Primary targets:
- prepared call-result producer code
- focused backend tests or prepared dump coverage

Actions:
- Publish or repair pointer result destination/home facts at the producer
  boundary identified in Step 2.
- Preserve callsite association, result value identity, destination stack slot
  or aggregate home, width, and pointer representation.
- Add focused coverage proving the prepared facts exist before RV64 object
  emission.
- Keep diagnostics fail-closed for missing, ambiguous, or mismatched authority.

Completion check:
- At least one in-scope representative row exposes complete pointer
  destination/home authority in prepared facts, and adjacent negative buckets
  remain excluded.

### Step 4: Admit Only Explicit Pointer Stack-Result Authority In RV64

Goal: let RV64 consumers handle the repaired pointer stack-result shape while
rejecting absent or ambiguous authority.

Primary targets:
- RV64 prepared-frame/call-result consumer code
- RV64 object emission diagnostics and tests

Actions:
- Consume the explicit prepared pointer result destination/home facts.
- Admit only the shape proven by Step 3.
- Reject rows with missing destination home, mismatched width, ambiguous source
  identity, non-ordinary call policy, or unrelated aggregate/FPR/local/global
  ownership.
- Add negative coverage for at least one missing-authority or wrong-bucket row.

Completion check:
- The RV64 consumer no longer rejects the repaired in-scope pointer
  stack-result row for missing policy/authority, and fail-closed diagnostics
  remain precise for unsupported adjacent rows.

### Step 5: Reclassify Representative Rows And Close Readiness

Goal: prove idea 627 is satisfied or record the next precise owner bucket.

Primary targets:
- all representative rows from the source idea
- `test_after.log`
- `todo.md`

Actions:
- Re-run the delegated backend proof command after the implementation packets.
- Re-check every representative row against the idea scope.
- Record which rows moved past the pointer stack-result authority blocker.
- Record any remaining failures as precise out-of-scope owner buckets rather
  than stretching this idea.

Completion check:
- `todo.md` states whether idea 627 is close-ready. If not close-ready, it
  identifies the remaining in-scope authority gap and the next executable step.
