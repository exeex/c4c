# Current Packet

Status: Active
Source Idea Path: ideas/open/528_bir_route6_call_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Route6 Call Publication Boundaries

## Just Finished

No executor packet has run for this active plan yet.

## Suggested Next

Supervisor can delegate Step 1 from `plan.md`: map route6 call publication
boundaries with AST-backed symbol queries before any body movement.

## Watchouts

- Keep this activation aligned to
  `ideas/open/528_bir_route6_call_publication_body_extraction.md`.
- Do not move route6 public declarations, route-index facade bodies, memory
  provenance headers, or implementation bodies outside the route6
  call-publication scope in the first mapping packet.
- Do not change call ABI lowering, LIR-to-BIR call generation, route1-route5
  behavior, or idea 422 producer capability.

## Proof

Lifecycle activation only; no build or test proof was run.
