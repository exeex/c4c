Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Lifecycle repair only: retired the stale idea `54` runbook after the source
idea added a prerequisite activation constraint, and activated idea `55` as the
current refactor route. No implementation packet has run on this plan yet.

## Suggested Next

Start `plan.md` Step 1 by re-reading `lir_to_bir_memory.cpp` and classifying
the remaining helper families into coordinator glue, value materialization,
local-slot mechanics, and any leftover provenance or addressing seams. Choose
one behavior-preserving second-layer extraction packet before editing code.

## Watchouts

- This plan is refactor-only; do not claim x86 backend capability progress from
  it.
- Keep `lower_scalar_or_local_memory_inst` in the main coordinator TU.
- Do not split by `local` versus `global` if the helper family is shared.
- Treat renderer de-headerization as separate idea `56`.

## Proof

No executor proof yet for the new active plan.
