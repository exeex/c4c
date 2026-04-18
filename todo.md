Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Activation only. `plan.md` now targets Step 1 for
`ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md`.

## Suggested Next

Execute `plan.md` Step 1 by reading
`src/backend/bir/lir_to_bir_memory.cpp` and
`src/backend/bir/lir_to_bir.hpp`, then write down a concrete first-pass
extraction map for `lir_to_bir_memory_addressing.cpp` and
`lir_to_bir_memory_provenance.cpp`.

## Watchouts

- Keep the first packet behavior-preserving; this runbook does not claim new
  backend capability.
- Do not force a `local` versus `global` split.
- Leave ambiguous helpers in `lir_to_bir_memory.cpp` for the first packet.
- Keep x86 renderer cleanup under idea `56`, not this runbook.

## Proof

Not run. Activation-only lifecycle change.
