# Prealloc Current-Block Routing Authority Closure Runbook

Status: Active
Source Idea: ideas/open/716_prealloc_current_block_routing_authority_closure.md
Resumed after completion of: ideas/closed/717_current_block_routed_value_authority_decomposition.md

## Purpose

Separate attachment, diagnostic identity, edge authority, and fixture semantics
before returning to bounded AArch64 consumption.

## Goal

Prove that only complete, invariant edge-derived prepared facts authorize a
current-block incoming-expression answer.

## Core Rule

Establish each authority boundary with its own registered contract. Route 5 is
diagnostic-only, attachment is not policy evidence, and the AArch64 integration
test is adopted only after the focused probes are green.

## Read First

- `ideas/open/716_prealloc_current_block_routing_authority_closure.md`
- `ideas/open/713_current_block_edge_bound_routing_consumption_decomposition.md`
- `review/step4_aarch64_consumption_review.md`
- `review/step4_aarch64_consumption_rereview.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`

## Current Targets

- Function-context ownership and prepared-lookup lifetime.
- Route 5 diagnostic-only enforcement.
- Complete edge-derived incoming-expression authority.
- Integration fixture policy-versus-attachment separation.
- Bounded AArch64 adoption after focused proof.

## Non-Goals

- Do not retire Route 5 payloads or resume broader idea 705 work.
- Do not use target reconstruction, function-wide scans, pointer identity,
  successor-only identity, or unique Route 5 selection as authority.
- Do not rewrite supported integration vectors or change fixture semantics to
  preserve them.
- Do not combine independent probes into the existing integration test.

## Execution Rules

- Establish a fresh registered baseline before semantic changes.
- Keep one primary contract per focused C++ probe under `tests/backend/bir/` or
  `tests/backend/mir/`.
- Prove authority without consulting Route 5 before target adoption.
- For every code-changing step run build, the named focused test, and the
  supervisor-selected matching proof command.
- Run fresh broader backend proof at the integration and handback checkpoints.

## Ordered Steps

### Step 1: Establish the authority-collision baseline

Goal: record the two rejected Step 4 routes and identify the first authoritative
fact each one lacks.

Actions:

- Preserve the owner-attachment correction as a candidate independent seam.
- Record where Route 5 currently becomes incoming-expression authority.
- Record how policy construction and lookup attachment are coupled in the
  integration fixture.
- Register or select the five primary contracts named by the source idea.

Completion check:

- Each collision maps to one owned seam and one registered primary contract;
  no AArch64 behavior change is attempted.

### Step 2: Prove owner attachment and lifetime independently

Goal: retain the valid ownership correction without granting routing authority.

Primary target:
`tests/backend/mir/backend_prealloc_current_block_lookup_attachment_lifetime_test.cpp`

Actions:

- Prove lookup construction occurs at the prealloc/function-context owner.
- Prove copied block contexts retain valid shared lookup lifetime.
- Prove missing attachment fails closed.
- Do not require Route 5 or a positive incoming-expression answer.

Completion check:

- The registered lifetime contract is green and independent of routing policy.

### Step 3: Enforce Route 5 diagnostic-only non-authority

Goal: make Route 5 unable to seed, replace, erase, or authorize routing facts.

Primary target:
`tests/backend/bir/backend_prealloc_route5_diagnostic_non_authority_test.cpp`

Actions:

- Exercise missing, unique, agreeing, and conflicting Route 5 diagnostics.
- Hold authoritative prepared edge facts constant while Route 5 varies.
- Remove any Route 5-to-`Available` synthesis or replacement path.

Completion check:

- The authoritative fact set and query status are unchanged by Route 5 input;
  build and focused proof are green.

### Step 4: Bind complete edge-derived incoming-expression authority

Goal: authorize `Available` only from complete and invariant prepared edge
facts.

Primary target:
`tests/backend/bir/backend_prealloc_current_block_incoming_expression_authority_test.cpp`

Actions:

- Carry predecessor, destination, source semantics, and semantic origin from
  the prepared edge fact into query authority.
- Require agreement across every applicable edge fact.
- Prove available, missing, ambiguous, and mismatched outcomes, including
  parallel predecessors and destinations.

Completion check:

- No successor-only or result-name collapse can produce `Available`; the full
  focused authority matrix is green without Route 5.

### Step 5: Separate fixture policy from attachment

Goal: make integration setup state exactly which policy exists and whether its
lookup is attached.

Primary target:
`tests/backend/mir/backend_aarch64_current_block_fixture_policy_attachment_test.cpp`

Actions:

- Provide independent policy-present and attachment-present controls.
- Prove a supported positive contains real complete edge authority.
- Prove detached and policy-absent cases fail closed as separate negatives.

Completion check:

- The registered fixture contract distinguishes all axes without changing the
  existing integration vectors.

### Step 6.1: Attach complete routing facts at the prepared owner boundary

Goal: make the already-queryable complete routing authority part of the
owner-bound `PreparedFunctionLookups` artifact before target consumption.

Actions:

- Add an owned current-block routing fact collection to
  `PreparedFunctionLookups` with the same function-context lifetime and copied
  block-context visibility proven by Step 2.
- Populate that collection during prepared lookup construction from the
  complete edge-derived facts proven by Step 4.
- Make the stable-key query read the owner-attached collection directly; do
  not require an external routing-fact vector at the consumption boundary.
- Preserve explicit missing, ambiguous, and mismatched outcomes and keep Route
  5 diagnostic-only.
- Extend the focused prepared-lookup contracts to prove attachment, query
  availability, and fail-closed behavior without AArch64 reconstruction.

Completion check:

- A `PreparedFunctionLookups` owner carries the complete routing fact
  collection and the stable-key query consumes it without an externally
  supplied vector; focused build and authority/lifetime proof are green.

### Step 6.2: Repair complete invariant owner-prepared fact coverage

Goal: prove that the owner-attached artifact covers the unchanged
incoming-expression contract from complete prepared edge semantics before any
target consumes it.

Primary targets:
`tests/backend/bir/backend_prealloc_current_block_incoming_expression_authority_test.cpp`
and the prepared-lookup focused contracts extended by Step 6.1.

Actions:

- Restore or prove the transfer-level destination invariant: an aggregate
  prepared transfer whose result disagrees with its selected edge or
  publication must fail closed rather than establish semantic origin.
- Define which routed values a complete prepared edge semantically authorizes;
  derive each value's source identity from that edge contract rather than
  rewriting source identity while appending a fact.
- Add a focused matrix for every value category required by the unchanged
  incoming-expression integration contract, including scalar operands and
  immediate-backed values if they are genuinely authorized, plus unrelated
  operands and mismatched transfer destinations as negative cases.
- Prove agreement across every applicable fact for predecessor, destination,
  source semantics, and semantic origin, including multiple and parallel edge
  facts for the same routed value.
- Keep fact preparation at the `PreparedFunctionLookups` owner boundary and
  keep Route 5, AArch64 inputs, result-name matching, and target-local scans out
  of authority construction.

Completion check:

- Focused authority and prepared-lookup contracts prove complete coverage for
  the unchanged supported incoming-expression cases and fail closed for
  incomplete, inconsistent, unrelated, ambiguous, and mismatched facts; no
  expectation rewrite or testcase-shaped authority synthesis is used.

### Step 6.3: Adopt bounded AArch64 consumption

Goal: consume the proven owner-attached query without reconstruction.

Primary target:
`tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`

Actions:

- Keep AArch64 limited to the stable consumption key and attached lookup.
- Remove local construction of routing facts from Route 5, MIR, value-home,
  publication-plan, or other target-local inputs.
- Preserve unchanged supported integration expectations.
- Run all focused contracts, the integration test, and fresh broader backend
  proof.

Completion check:

- Focused, integration, and broader proof are green; AArch64 contains no
  authority builder or fallback; no positive depends on Route 5.

### Step 7: Hand back to idea 713

Goal: return the closed authority contract to the still-open bounded-consumption
initiative.

Actions:

- Record durable completed contract and proof in idea 716.
- Switch lifecycle execution back to idea 713 at its Step 4 boundary.
- Keep idea 705 blocked until idea 713 completes its handback.

Completion check:

- Idea 716 is ready to close and idea 713 can resume without unresolved Route 5,
  edge-identity, attachment, or fixture-policy ambiguity.
