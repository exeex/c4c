# Current Packet

Status: Active
Source Idea Path: ideas/open/529_bir_route_facade_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Route Facade Boundaries

## Just Finished

Activation created the active runbook for Step 1: Map Route Facade Boundaries.

## Suggested Next

Map the exact route facade body set in `src/backend/bir/bir.cpp` using
clang-backed symbol queries before moving implementation code.

## Watchouts

- Do not edit implementation files during the mapping packet.
- Do not move route4, route7, validation, fused-compare, or
  materialized-condition internals as facade work.
- Keep public declarations in `src/backend/bir/bir.hpp`.

## Proof

Lifecycle activation only; no build or tests required.
