# BIR Route Header Split Resume Runbook

Status: Active
Source Idea: ideas/open/530_bir_route_header_split_after_body_moves.md
Activated from: parked route-header split after close commit `779d4f059`

## Purpose

Resume the parked BIR route declaration header split after the route-index
standalone prerequisite route was resolved.

## Goal

Decide whether the existing route-header split work is now complete enough to
close idea 530, or identify exactly one remaining narrow route declaration
boundary that can move without increasing include burden.

## Core Rule

This is behavior-preserving header work only. Do not change route semantics,
implementation-body ownership, public names, namespaces, signatures, enum
values, or test expectations.

## Read First

- `ideas/open/530_bir_route_header_split_after_body_moves.md`
- `review/bir_route_header_split_review.md`
- recent close commit `779d4f059`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir_route_index.hpp`
- `src/backend/bir/bir_route_index_prereqs.hpp`
- route owner files under `src/backend/bir/`
- include users under `src/backend/bir/`, `src/backend/mir/`, and
  `src/backend/prealloc/`
- `.codex/skills/c4c-clang-tools/` for declaration, reference, and include
  evidence before selecting any header boundary

## Current Targets

- Route declaration clusters still in `src/backend/bir/bir.hpp`
- The existing route-index fragment and prerequisite split:
  `src/backend/bir/bir_route_index.hpp` and
  `src/backend/bir/bir_route_index_prereqs.hpp`
- Any remaining focused route header candidate that can reduce or clarify
  dependencies without becoming a new catch-all include

## Non-Goals

- Do not move implementation bodies.
- Do not move `Value`, `Inst`, `Block`, `Function`, `Module`,
  `MemoryAddress`, or route1 identity ownership.
- Do not force `bir_route_index.hpp` to become standalone by changing
  `RouteIndexRecordReference` layout or route1 source identity ownership.
- Do not replace broad includes where consumers still need broad BIR model or
  route surfaces.
- Do not combine this idea with memory-provenance or local-array semantic-GEP
  header-readiness work from later ordered ideas.

## Working Model

- `bir.hpp` remains the compatibility aggregator.
- `bir_route_index.hpp` has been proven aggregator-only for now because
  `RouteIndexRecordReference` stores `Route1SourceValueIdentity` by value.
- `bir_route_index_prereqs.hpp` is a valid prerequisite declaration split, but
  direct include replacement is not safe unless a new probe proves otherwise.
- Idea 530 should close if no remaining route declaration boundary can reduce
  or clarify dependencies without pulling in later memory-provenance or
  semantic-GEP work.

## Execution Rules

- Start with a mapping-only packet and record the result in `todo.md` before
  any header edit.
- Prefer clang-backed symbol, signature, caller/callee, and type-reference
  queries over manual long-file inspection.
- Use direct include probes only as evidence; do not pursue direct include
  replacement when it reconstructs the old broad `bir.hpp` surface.
- Move declarations only in small groups with explicit complete-type and
  include-user evidence.
- Treat semantic diffs, body movement, expectation weakening, or public API
  changes as blockers.
- Ask the supervisor for broader backend proof if any header churn reaches
  MIR, prealloc, or broad backend consumers.

## Step 1: Map Remaining Route Header Candidates

Goal: determine whether idea 530 has any safe remaining route declaration
movement after the route-index prerequisite closure.

Primary target: route declarations in `src/backend/bir/bir.hpp` plus the
current `bir_route_index*` headers.

Actions:

- Use clang-backed symbol, signature, type-reference, and include/user queries
  to map remaining route declaration clusters.
- Reconfirm that `bir_route_index.hpp` remains aggregator-only unless a direct
  top-level include probe now passes without route1 identity/layout work.
- Identify whether any non-route-index declaration cluster can move without
  pulling in `Value`, `Inst`, `Block`, `Function`, `Module`, `MemoryAddress`,
  memory provenance, local-array, or semantic-GEP ownership.
- Record one of these outcomes in `todo.md`:
  - close recommendation because the route-header idea is satisfied or no
    safe remaining movement exists;
  - exactly one candidate header boundary with declarations to move, required
    includes, include-site constraints, and proof command recommendation.

Completion check:

- `todo.md` contains the remaining-candidate map, route-index status, selected
  next action, rejected boundaries, dependency risks, and proof recommendation.
- No implementation or header files are edited in this mapping step.

## Step 2: Execute One Boundary Or Park For Closure

Goal: perform the single safe action selected by Step 1.

Primary target: the candidate focused route header selected by Step 1, or
`todo.md` lifecycle notes if Step 1 recommends closure without more code.

Actions:

- If Step 1 selected a header boundary, move only those declarations and keep
  `bir.hpp` as the compatibility aggregator.
- Preserve spelling, namespaces, signatures, enum values, layout, and behavior.
- Do not edit implementation bodies.
- Replace include sites only when Step 1 proves the consumer avoids the broad
  BIR surface.
- If Step 1 selected closure, do not edit headers; record the proof-backed
  closure rationale in `todo.md`.
- Run the supervisor-delegated proof command exactly after any header edits
  and write results to `test_after.log`.

Completion check:

- Header-edit path: build and focused backend proof pass, and the diff is only
  declaration/header/include movement.
- Closure path: `todo.md` records why no further header movement is safe or
  necessary under idea 530.

## Step 3: Lifecycle Checkpoint

Goal: decide whether idea 530 can close, should remain parked, or needs a
  separately owned follow-up idea.

Actions:

- Compare final declarations and include edits against Step 1 evidence.
- Confirm no implementation bodies, core model types, route semantics, public
  API names, namespaces, signatures, enum values, or tests changed.
- Confirm later ordered ideas remain separate:
  memory provenance in idea 531 and local-array/semantic-GEP in idea 532.
- Ask the supervisor to select close validation if source-idea completion is
  supported.

Completion check:

- `todo.md` records the lifecycle recommendation and proof state.
- The source idea is either ready for close delegation or has a concrete,
  non-overlapping next packet.
