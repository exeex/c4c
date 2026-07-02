# BIR Route Facade Body Extraction Runbook

Status: Active
Source Idea: ideas/open/529_bir_route_facade_body_extraction.md

## Purpose

Extract cross-route facade implementation bodies from `src/backend/bir/bir.cpp`
after the route body owners they bridge have stabilized.

## Goal

Move existing route-index facade bodies into a focused implementation owner
without changing validation, fused-compare, materialized-condition, route4, or
route7 semantics.

## Core Rule

This is behavior-preserving body movement only. Do not use the facade move to
alter public APIs, route ownership, validation decisions, or materialized
condition behavior.

## Read First

- `ideas/open/529_bir_route_facade_body_extraction.md`
- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir.hpp`
- Current route4 and route7 implementation owners after their body extractions
- `.codex/skills/c4c-clang-tools/` when mapping callers, callees, and type
  references

## Current Targets

- Cross-route route-index facade implementation bodies currently still in
  `src/backend/bir/bir.cpp`
- Facade glue for validation, fused compare, and materialized-condition
  consumers
- Optional new `bir_route_facade.cpp` file when the mapped body set justifies
  a dedicated owner
- Private helper declarations only when required to preserve existing behavior
  after moving bodies

## Non-Goals

- Do not move core route declarations out of `src/backend/bir/bir.hpp`.
- Do not split public declarations as part of this idea.
- Do not move route4 or route7 owned bodies to satisfy the facade extraction.
- Do not alter fused compare, materialized-condition, validation, route4, or
  route7 semantics.
- Do not collapse facade validation into `bir_validate.cpp`.
- Do not combine this work with route6 extraction or any header split idea.

## Working Model

- The facade is late cross-route glue. It should depend on stable route owners,
  not become a catch-all implementation file.
- The initial packet should map the exact body set and dependencies before any
  move.
- Later packets should move only confirmed facade bodies, keeping public
  declarations in place and adding private declarations only where the compiler
  requires them.

## Execution Rules

- Keep each implementation packet small enough to prove with build plus focused
  route-index validation, fused-compare, and materialized-condition tests.
- Prefer `c4c-clang-tool` or `c4c-clang-tool-ccdb` symbol queries over manual
  large-file inspection when mapping callers, callees, definitions, and type
  references.
- Preserve public declarations in `src/backend/bir/bir.hpp`.
- Keep private helper exposure minimal and local to the moved body owner.
- Treat any changed validation failure, source identity, fused compare result,
  or materialized-condition decision as a blocker, not cleanup progress.
- Escalate to supervisor for broader backend proof if multiple route body moves
  have landed since the last broad validation.

## Step 1: Map Route Facade Boundaries

Goal: identify the exact facade bodies and dependency edges before moving code.

Primary target: `src/backend/bir/bir.cpp` facade implementation bodies and
their existing public declarations in `src/backend/bir/bir.hpp`.

Actions:

- Use clang-backed symbol queries to list route-index facade definitions still
  in `src/backend/bir/bir.cpp`.
- Record direct callers, callees, type references, and route4/route7
  dependencies for each candidate body.
- Separate true facade glue from route4-owned bodies, route7-owned bodies,
  validation internals, fused-compare internals, and materialized-condition
  internals.
- Decide whether `bir_route_facade.cpp` is needed for the mapped body set.
- Record likely private helper declarations needed by the move.

Completion check:

- `todo.md` names the exact bodies to move, bodies not to move, direct
  dependency edges, likely declarations, and focused proof recommendations for
  the body-move packet.
- No implementation files are edited in this mapping step.

## Step 2: Move Confirmed Facade Bodies

Goal: move only confirmed facade implementation bodies out of
`src/backend/bir/bir.cpp`.

Primary target: `src/backend/bir/bir_route_facade.cpp` if created, plus
build-system registration for the new translation unit if required.

Actions:

- Create `bir_route_facade.cpp` only if Step 1 confirms a focused facade body
  set.
- Move the confirmed facade bodies without changing signatures or public
  declarations.
- Add the minimal includes and private declarations needed for the moved
  bodies to compile.
- Keep route4, route7, validation, fused-compare, and materialized-condition
  implementation bodies in their existing owners.
- Run the supervisor-delegated proof command exactly and write results to
  `test_after.log`.

Completion check:

- Build proof passes.
- Focused route-index validation, fused-compare, and materialized-condition
  proof passes.
- Diff shows body movement and mechanical owner wiring only, with no semantic
  changes or public declaration split.

## Step 3: Facade Extraction Review Checkpoint

Goal: verify the move stayed narrow and decide whether broader validation is
needed.

Actions:

- Inspect the diff for route drift, public API changes, and catch-all facade
  growth.
- Compare the final moved body set against the Step 1 mapping.
- Confirm no route4 or route7 API was changed to make the move compile.
- Ask the supervisor to choose broader backend validation if recent route body
  moves make focused proof insufficient.

Completion check:

- `todo.md` records focused proof results and any supervisor-selected broader
  validation result.
- The active source idea can be closed only if the facade body set is moved and
  the proof scope is accepted.
