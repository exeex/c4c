# Current Packet

Status: Active
Source Idea Path: ideas/open/530_bir_route_header_split_after_body_moves.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Remaining Route Header Candidates

## Just Finished

Lifecycle activation created this scratchpad for Step 1 of `plan.md`.

## Suggested Next

Delegate Step 1 as a mapping-only packet. The executor should map remaining
route header candidates, reconfirm the route-index aggregator-only state, and
recommend either closure or one exact safe header boundary.

## Watchouts

- Do not edit implementation or header files during Step 1.
- Do not force `bir_route_index.hpp` standalone by moving route1 identity
  ownership or changing `RouteIndexRecordReference` layout.
- Keep memory-provenance and local-array/semantic-GEP header work for later
  ordered ideas.

## Proof

Activation only; no build or tests required.
