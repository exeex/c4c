# BIR Route Facade Body Extraction

Status: Closed
Type: Behavior-preserving cleanup
Parent: `ideas/closed/518_bir_core_model_cleanup_umbrella.md`
Order: BIR cleanup follow-up 10 of 13, after `ideas/open/528_bir_route6_call_publication_body_extraction.md`
Owning Layer: BIR cross-route facade glue
Source Artifact: `docs/bir_core_cleanup/follow_up_ideas.md`

## Goal

Move cross-route facade implementation bodies from `bir.cpp` as a late glue
slice after route4 and route7 body owners are stable.

## Why This Exists

The facade is cross-route glue for validation, fused compare, and
materialized-condition consumers. It should move only after the routes it
bridges have focused implementation owners.

## In Scope

- Move existing route-index facade bodies only.
- Add `bir_route_facade.cpp` and private helper declarations only if needed.
- Preserve public declarations in `bir.hpp`.
- Use `.codex/skills/c4c-clang-tools/` to map facade callers/callees and
  route4/route7 dependencies.

## Out Of Scope

- Do not move core route declarations.
- Do not alter route4, route7, fused compare, or materialized-condition
  semantics.
- Do not collapse facade validation into `bir_validate.cpp`.
- Do not combine this with route6 extraction.

## Acceptance Criteria

- Build proof passes.
- Focused route-index validation, fused-compare, and materialized-condition
  proof passes.
- Supervisor considers a broader backend subset if multiple route body moves
  have landed since the last broad proof.

## Completion Notes

Closed after Step 1 mapped the route-index facade boundary and Step 2 moved
the seven confirmed facade bodies into `src/backend/bir/bir_route_facade.cpp`.
Public declarations remained in `src/backend/bir/bir.hpp`, route4 and route7
owners were not reshaped for the move, and the close gate passed with the
focused backend subset selected by the supervisor.

## Reviewer Reject Signals

- Facade movement changes validation failures, source identity, or
  materialized-condition decisions.
- The facade becomes a new catch-all for unrelated route bodies.
- Route4 or route7 APIs change to satisfy the move.
- Public declarations are split without a separate header idea.
