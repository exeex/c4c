# Current Packet

Status: Active
Source Idea Path: ideas/open/525_bir_route3_memory_access_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map route3 memory-access symbols and dependencies

## Just Finished

- Lifecycle activation only; no executor packet has run yet.

## Suggested Next

- Delegate Step 1 from `plan.md`: map route3 memory-access symbols,
  dependencies, route6 consumers, and the focused proof subset before code
  movement.

## Watchouts

- Keep public route3 declarations in `bir.hpp`.
- Preserve route6 access to the same route3 facts.
- Do not move memory authority records into private lowering files.

## Proof

- Not run; lifecycle-only activation.
