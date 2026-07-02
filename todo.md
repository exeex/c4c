# Current Packet

Status: Active
Source Idea Path: ideas/open/533_bir_route_index_standalone_prerequisites.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Route-Index Prerequisites

## Just Finished

Lifecycle switched away from idea 530 after reviewer report
`review/bir_route_header_split_review.md`. Idea 530 is parked because the
current `bir_route_index.hpp` file is an aggregator-included declaration
fragment, not a standalone narrow dependency header, and Step 3 direct include
replacement was proven unsafe.

## Suggested Next

Step 1 should run a mapping-only packet for
`bir_route_index.hpp` standalone prerequisites. Record:

- the direct top-level include compile failure map
- required prerequisite declarations and standard includes
- complete-type risks around `Function`, `Block`, `Value`, and vectors or
  dereferenced model data
- route1, route4, and route7 declaration clusters that block standalone use
- include users that still need broad `bir.hpp`
- the first safe prerequisite boundary, or a proof-backed aggregator-only
  recommendation

## Watchouts

- Do not continue idea 530 Step 3-style direct include replacement attempts.
- Do not claim dependency reduction while `bir_route_index.hpp` still cannot
  compile as a top-level include.
- Keep `bir.hpp` as compatibility aggregator unless a consumer is proven not to
  need the broad model or route surface.
- Avoid broad declaration movement; only split mapped prerequisite clusters.

## Proof

Lifecycle-only rewrite; no build/tests required. No implementation files were
edited.
