# Current Packet

Status: Active
Source Idea Path: ideas/open/524_bir_route4_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map Route4 Ownership And Dependencies

## Just Finished

Lifecycle activation created the active runbook and execution scratchpad for
Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: use `c4c-clang-tools` to map route4 publication
symbols, callers, callees, and route6/facade consumers before moving bodies.

## Watchouts

- Keep this as behavior-preserving body extraction.
- Do not move public route4 declarations out of `bir.hpp`.
- Do not move route-index facade bodies, route6 source-selection policy, or
  route-specific validation records.
- Do not rewrite expectations or weaken tests as proof.

## Proof

Lifecycle-only activation; no build or CTest proof required for this slice.
