# Current Packet

Status: Active
Source Idea Path: ideas/open/526_bir_route5_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Route5 Boundaries

## Just Finished

Activation created the runbook and executor-compatible state for Step 1.

## Suggested Next

Delegate Step 1 to an executor: map route5 symbols, route1/route3/route4
dependencies, and route6 consumers before any body move.

## Watchouts

- Keep this slice behavior-preserving.
- Do not move route5 public declarations out of `bir.hpp`.
- Do not duplicate route3 or route4 matching logic inside route5.
- Do not combine this with route6 extraction.

## Proof

Not run; lifecycle-only activation.
