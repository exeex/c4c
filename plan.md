# Route 6 Call-Use Source Adapter Runbook

Status: Active
Source Idea: ideas/open/205_route6_call_use_source_adapter.md

## Purpose

Activate the Route 6 follow-up by migrating one selected call instruction and
one argument or result source role to semantic source identity from BIR records
while keeping prepared call-plan fallback for ABI and layout policy.

Goal: one selected call-use reader obtains only Route 6 semantic source identity
from BIR, with prepared call-plan authority preserved for every ABI-bound,
layout-bound, and policy-sensitive fact.

Core Rule: Route 6 may provide semantic call-use source identity only; it must
not become the authority for call-plan construction, ABI placement, wrappers,
clobbers, outgoing stack sizing, byval lanes, variadic FPR counts, helper
protocols, value homes, move bundles, aggregate transport, publication routing
policy, or final call records.

## Read First

- `ideas/open/205_route6_call_use_source_adapter.md`
- Existing Route 6 record/index definitions and publication sites
- Existing prepared call-plan readers, `PreparedCallPlanLookups`, helper
  protocols, and fallback helpers
- Nearby tests for prepared call lookup, call-boundary effects, call source
  identity, helper/direct-call diagnostics, and AArch64/x86 output stability

## Current Scope

- Select exactly one call instruction and exactly one argument or result source
  role for the first Route 6 adapter.
- Use Route 6 records or indexes only for source value/base value/name,
  materializable same-block producer, direct-global dependency,
  publication-source identity when semantic, call-site ordinal, and selected
  result-lane source identity when applicable.
- Preserve prepared call-plan fallback for ABI placement, call wrapper kind,
  clobbers, outgoing stack sizing, byval lanes, variadic FPR counts,
  helper/carrier protocols, value homes, move bundles, aggregate transport
  layout, publication routing policy, and final call records.

## Non-Goals

- Do not replace whole call plans or `PreparedCallPlanLookups`.
- Do not move ABI placement, call wrapper kind, clobbers, outgoing stack sizing,
  byval lanes, variadic FPR counts, helper/carrier protocols, value homes, move
  bundles, aggregate transport layout, publication routing policy, or final
  call records into BIR schema.
- Do not delete, privatize, or rename prepared call helpers as part of this
  adapter.
- Do not weaken direct-call/helper oracle messages, expected output, or
  unsupported-test contracts.

## Working Model

- Treat Route 6 as a semantic identity adapter for one call-use source role.
- Treat prepared call plans as the authority for ABI, layout, helper protocol,
  policy, storage, movement, and emitted output.
- Fail closed when Route 6 facts are absent, mismatched, duplicated,
  ambiguous, ABI-bound, policy-bound, or not applicable to the selected
  instruction and role.
- Keep the first implementation narrow: one instruction, one source role, one
  proof ladder, then supervisor review before broadening.

## Execution Rules

- Keep implementation steps small enough for build plus targeted backend tests.
- Prefer existing helper APIs and naming near the selected call reader.
- Add no testcase-shaped shortcuts, named-fixture branches, or string rewrites
  that mask unchanged capability.
- If diagnostics or output strings are touched, prove message/output authority
  and adjacent direct-call/helper behavior.
- After each code-changing packet, run the supervisor-delegated build/test
  command exactly and record proof in `todo.md`.
- Escalate to broader validation before claiming the adapter complete if the
  selected boundary affects shared call-plan or target-output behavior.

## Ordered Steps

### Step 1: Select The Route 6 Adapter Boundary

Goal: identify exactly one call instruction and one argument or result source
role for the first Route 6 semantic identity adapter.

Primary target: one existing prepared call-use reader.

Actions:

- Inspect Route 6 publication records and existing prepared call reader call
  sites.
- Choose the smallest boundary that needs semantic call-use source identity and
  already has prepared fallback for ABI/layout-sensitive behavior.
- Map which facts can come from Route 6 and which facts must remain prepared.
- Identify the targeted tests and any missing negative coverage.

Completion check:

- `todo.md` names one selected call instruction, one selected source role, the
  Route 6 facts it may read, the prepared facts it must keep, and the narrow
  proof command for Step 2.

### Step 2: Add The Fail-Closed Route 6 Reader Adapter

Goal: introduce the adapter path without changing prepared call-plan authority.

Primary target: the selected reader and its nearest Route 6 lookup helpers.

Actions:

- Add or reuse a helper that validates the selected Route 6 call-use source
  identity.
- Reject absent facts, mismatched source IDs, mismatched call-site ordinals,
  duplicate or ambiguous facts, ABI-bound facts, policy-bound facts, and missing
  materializable producer cases for the selected boundary.
- Return to prepared fallback for ABI placement, wrappers, clobbers, outgoing
  stack sizing, byval lanes, variadic FPR counts, helper protocols, homes, move
  bundles, aggregate transport, publication routing policy, and final call
  records.
- Keep existing prepared helper groups and broad lookup surfaces intact.

Completion check:

- The project builds.
- Targeted proof covers route/prepared source-id agreement, absent facts,
  mismatched source-id fallback, ambiguity rejection, ABI-bound rejection, and
  prepared fallback for the selected boundary.

### Step 3: Wire The Selected Reader To The Adapter

Goal: make the selected reader consume Route 6 semantic source identity when
valid and prepared fallback when invalid or policy-bound.

Primary target: the selected reader from Step 1.

Actions:

- Thread the validated Route 6 semantic identity into the selected reader.
- Keep prepared ABI placement, wrappers, clobbers, outgoing stack sizing, byval
  lanes, variadic FPR counts, helper protocols, homes, move bundles, aggregate
  transport, publication routing policy, final call records, and output
  authority unchanged.
- Preserve AArch64 and x86 output stability unless the source idea explicitly
  justifies a visible diagnostic or oracle adjustment.
- Add focused tests for success, absent fallback, source-id mismatch,
  ambiguity, ABI-bound rejection, prepared fallback, and unchanged output.

Completion check:

- The selected reader obtains only Route 6 semantic call-use source identity
  from BIR.
- Prepared call plans remain visibly authoritative for ABI/layout and policy
  behavior.
- Targeted tests pass with no expected-output weakening.

### Step 4: Prove Adapter Completeness And Route Quality

Goal: establish that the adapter is real capability progress and not a narrow
fixture shortcut.

Primary target: tests and review evidence for the selected adapter boundary.

Actions:

- Run the supervisor-selected build and targeted backend test subset.
- Add or run nearby same-feature cases to show the adapter is not
  testcase-shaped.
- Include direct-call/helper oracle sanity when the selected boundary is near
  those diagnostics.
- Document remaining non-goals and any proposed next Route 6 boundary in
  `todo.md` without expanding this plan.

Completion check:

- Proof covers source-id agreement, absent facts, mismatch, ambiguity,
  ABI-bound rejection, prepared fallback, and unchanged AArch64/x86 output.
- No whole-call-plan replacement, ABI/layout migration, helper deletion,
  expected-string weakening, or unsupported downgrade is present.
- The supervisor can decide whether to accept the slice, request reviewer
  scrutiny, or activate a separate follow-up idea.
