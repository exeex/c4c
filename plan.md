# BIR Route Index Standalone Prerequisites Runbook

Status: Active
Source Idea: ideas/open/533_bir_route_index_standalone_prerequisites.md

## Purpose

Make the existing route-index declaration boundary either standalone and safe
to include directly, or explicitly prove that it must remain an internal
`bir.hpp` fragment.

## Goal

Resolve the reviewer-identified route drift from idea 530 without forcing
unsafe direct include replacement.

## Core Rule

This is behavior-preserving prerequisite mapping and declaration movement only.
Do not change route semantics, public signatures, enum values, or
implementation-body ownership.

## Read First

- `ideas/open/533_bir_route_index_standalone_prerequisites.md`
- `ideas/open/530_bir_route_header_split_after_body_moves.md`
- `review/bir_route_header_split_review.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir_route_index.hpp`
- `src/backend/bir/bir_route_facade.cpp`
- `src/backend/bir/bir_route1.cpp`
- `src/backend/bir/bir_route4_publication.cpp`
- `src/backend/bir/bir_route7_comparison.cpp`
- Include users under `src/backend/bir/`, `src/backend/mir/`,
  `src/backend/prealloc/`, and direct-source BIR tests
- `.codex/skills/c4c-clang-tools/` for declaration, reference, and include
  evidence before selecting prerequisite boundaries

## Current Targets

- `src/backend/bir/bir_route_index.hpp`
- The prerequisite route/model declarations that currently make that file
  namespace- and order-dependent on `bir.hpp`
- Candidate focused prerequisite headers under `src/backend/bir/`
- Include sites only after the route-index header compiles as a top-level
  include

## Non-Goals

- Do not move implementation bodies.
- Do not move ownership of `Value`, `Inst`, `Block`, `Function`, `Module`, or
  `MemoryAddress`.
- Do not create a new catch-all route monolith or a renamed aggregator-only
  substitute.
- Do not change public API names, namespaces, signatures, enum values, or
  behavior.
- Do not replace broad includes in consumers that still need broad BIR model or
  route surfaces.
- Do not combine this idea with memory-provenance or local-array semantic-GEP
  header-readiness work.

## Working Model

- `bir_route_index.hpp` is currently an aggregator-included declaration
  fragment, not a standalone public header.
- It depends on earlier `bir.hpp` declarations for core BIR model types,
  route1 source identity, route4 publication records/statuses, route7
  comparison records/statuses, `BlockLabelId`, and standard library types.
- `bir.hpp` remains the compatibility aggregator while prerequisites are
  mapped or split.
- A direct include replacement is not progress unless the target consumer avoids
  reconstructing the old broad `bir.hpp` surface with several new headers.

## Execution Rules

- Start with a mapping-only packet. Record top-level include failures,
  prerequisite declarations, complete-type dependencies, current include users,
  and candidate prerequisite boundaries in `todo.md` before editing headers.
- Prefer clang-backed symbol and type-reference queries over manual
  large-file inspection when mapping declarations and consumers.
- Use direct no-file compile probes for `#include
  "src/backend/bir/bir_route_index.hpp"` as the standalone-readiness signal.
- Move declarations only in small prerequisite groups that can be proved by
  build plus focused backend route tests.
- Keep `bir.hpp` as a stable aggregator unless a direct consumer is proven safe
  for a narrower include.
- Treat any semantic diff, body movement, test expectation weakening, or public
  signature change as a blocker.
- Escalate to supervisor for broader backend proof if prerequisite splitting
  touches broad model declarations or include churn reaches MIR/prealloc
  consumers.

## Step 1: Map Route-Index Prerequisites

Goal: identify exactly why `bir_route_index.hpp` is not standalone and which
dependencies can be split safely.

Primary target: `src/backend/bir/bir_route_index.hpp` and prerequisite
declarations currently supplied earlier by `src/backend/bir/bir.hpp`.

Actions:

- Run clang-backed symbol, signature, and type-reference queries for
  `bir_route_index.hpp` and its referenced route-index functions/types.
- Run a direct top-level include compile probe for `bir_route_index.hpp` and
  record the missing declarations/includes.
- Map dependencies on `Function`, `Block`, `Value`, `BlockLabelId`,
  `Route1SourceValueIdentity`, route4 publication records/statuses, and route7
  comparison records/statuses.
- Inspect current broad include users under `src/backend/bir/`,
  `src/backend/mir/`, `src/backend/prealloc/`, and focused BIR tests.
- Decide whether the next safe packet is a prerequisite declaration split,
  namespace/include hardening for `bir_route_index.hpp`, or a proof-backed
  aggregator-only decision.
- Record candidate header names, declarations to move, declarations to leave,
  direct include-site constraints, and proof recommendations in `todo.md`.

Completion check:

- `todo.md` contains the standalone failure map, prerequisite declaration
  clusters, complete-type risks, safe first boundary, include-site constraints,
  and proof command recommendation.
- No implementation or header files are edited in this mapping step.

## Step 2: Split One Proven Prerequisite Boundary

Goal: remove one mapped prerequisite blocker without changing behavior or
creating a new broad route aggregator.

Primary target: the focused prerequisite header selected by Step 1, plus
`bir.hpp` and `bir_route_index.hpp` includes only as needed.

Actions:

- Move only the prerequisite declarations approved by Step 1.
- Preserve spelling, namespace as observed through `bir.hpp`, signatures, enum
  values, and behavior.
- Add minimal namespace wrappers, standard includes, and focused prerequisite
  includes needed for the selected boundary.
- Keep `bir.hpp` as the compatibility aggregator.
- Do not move implementation bodies.
- Run the supervisor-delegated proof command exactly and write results to
  `test_after.log`.

Completion check:

- Build proof passes.
- Focused BIR route tests chosen by the supervisor pass.
- The direct top-level include probe for `bir_route_index.hpp` is either
  closer to standalone with fewer mapped blockers or fully passes.
- Diff shows declaration/header/include movement only.

## Step 3: Promote or Park Route-Index Standalone Use

Goal: decide whether `bir_route_index.hpp` is now a safe narrow dependency
header or should remain explicitly aggregator-only.

Actions:

- Re-run the direct top-level include compile probe for `bir_route_index.hpp`.
- If the probe passes, inspect candidate consumers and replace broad `bir.hpp`
  includes only where the consumer does not need broad BIR model or route
  surfaces.
- If the probe still fails or include replacements would reconstruct the old
  broad surface, record an aggregator-only decision in `todo.md` and stop
  include churn.
- Run the supervisor-delegated proof command exactly after any code/header
  edits and write results to `test_after.log`.

Completion check:

- `todo.md` records either safe standalone include proof plus any include-site
  replacements, or a proof-backed aggregator-only decision.
- No consumer requires several new route/model headers just to recover the old
  `bir.hpp` surface.
- Build and focused route proof pass after any edits.

## Step 4: Review and Lifecycle Checkpoint

Goal: decide whether this prerequisite idea can close and whether idea 530 can
resume or close with clearer dependency evidence.

Actions:

- Compare final declarations, include edits, and probe results against Step 1
  mapping.
- Confirm no implementation bodies, route semantics, public API names,
  signatures, or enum values changed.
- Ask the supervisor to choose broader backend validation if header churn
  reached broad backend consumers.
- Record whether idea 530 should resume for additional header splits, close as
  a narrow clarification, or remain parked.

Completion check:

- `todo.md` records focused proof results and any supervisor-selected broader
  validation result.
- The active source idea can close only if the route-index standalone question
  is answered with proof.
