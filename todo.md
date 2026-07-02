# Current Packet

Status: Active
Source Idea Path: ideas/open/530_bir_route_header_split_after_body_moves.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Route Header Boundaries

## Just Finished

Activation created the active runbook for Step 1: Map Route Header Boundaries.

## Suggested Next

Map route declarations in `src/backend/bir/bir.hpp` using clang-backed symbol
and type-reference queries before editing headers.

## Watchouts

- Do not edit implementation or header files during the mapping packet.
- Do not move `Value`, `Inst`, `Block`, `Function`, `Module`, or
  `MemoryAddress`.
- Do not create a new catch-all route monolith or change public signatures.
- Keep body movement, memory-provenance readiness, and local-array
  semantic-GEP readiness out of this idea.

## Proof

Lifecycle activation only; no build or tests required.
