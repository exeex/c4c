# Direct-Edge Publication Available-Move Contract Runbook

Status: Active
Source Idea: ideas/open/722_direct_edge_publication_available_move_contract.md
Activated after parking: ideas/open/708_x86_named_handoff_materializer_cleanup.md

## Purpose

Repair the common prepared-MIR producer/query gap that blocks route-independent
x86 edge-publication consumption.

## Goal

Publish coherent typed move authority for supported direct-edge register,
immediate, and memory sources while preserving fail-closed negative states.

## Core Rule

Availability must come from genuine producer-owned publication, move, source,
and freshness facts. Tests and targets may observe this contract but must not
synthesize or reconstruct it.

## Read First

- `ideas/open/722_direct_edge_publication_available_move_contract.md`
- `ideas/open/708_x86_named_handoff_materializer_cleanup.md`
- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
- `ideas/closed/692_prepared_mir_source_dependency_freshness_view_contract.md`
- `ideas/closed/717_prepared_mir_join_source_identity_completion.md`

## Current Scope

- Prepared direct-edge publication production and typed query admission.
- Register, immediate, and memory source fact continuity.
- Focused positive and fail-closed producer/query proof.

## Non-Goals

- Do not edit x86 emission or resume idea 708's blocked consumer slice.
- Do not restore Route 5 compatibility authority.
- Do not weaken supported expectations or inject readiness only in fixtures.
- Do not absorb Route 3, joined-branch, ABI, or target scheduling work.

## Execution Rules

- Localize the first missing/rejected fact before changing production.
- Repair a semantic producer rule, never a testcase-shaped path.
- Keep every negative admission state precise and fail closed.
- Prove more than the original register-source fixture shape.
- Run focused proof per code step and broader matching validation at acceptance.

## Ordered Steps

### Step 1: Localize the unavailable direct-edge move fact

Goal: identify the earliest producer or typed-admission boundary that prevents
the supported register-source publication from becoming `Available`.

Actions:

- Trace publication, move, source, producer, storage, and freshness facts into
  `current_block_direct_edge_publication_sources`.
- Compare the failing supported shape with an available nearby shape.
- Record the first missing or contradictory fact and its owning producer.

Completion check:

- The first bad fact, owner, expected invariant, and focused positive/negative
  proof surfaces are documented without changing x86 emission.

### Step 2: Repair general producer and admission continuity

Goal: make genuine supported direct-edge moves available through the typed
prepared-MIR view.

Actions:

- Publish or preserve the missing producer-owned fact at its earliest correct
  boundary.
- Keep exact edge, destination, source, move, producer, storage, publication,
  and freshness identity aligned.
- Preserve precise rejection for incomplete, stale, ambiguous, mismatched, or
  unsupported evidence.

Completion check:

- Supported register-source authority is `Available` through a general rule,
  and focused negative states remain fail closed.

### Step 3: Prove adjacent source families and acceptance

Goal: demonstrate that the repaired contract is semantic rather than shaped to
one fixture.

Actions:

- Add or extend positive coverage for register, immediate, and memory sources.
- Add nearby missing, stale, ambiguous, mismatch, unsupported, and incomplete
  authority coverage.
- Run the supervisor-selected focused build/tests and broader matching backend
  regression comparison.

Completion check:

- Focused and broader proof are green without expectation changes, fixture-only
  injection, target fallback, or the original no-`Available` failure mode.
