Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished
- None yet. Activation only.

## Suggested Next
- Plan Step: Step 1 - inspect the prepared control-flow handoff in
  `src/backend/prealloc/*` and `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
  to choose the authoritative branch-condition and join-transfer contract
  surfaces.

## Watchouts
- Do not treat x86 matcher widening as progress for this idea.
- Keep semantic transfer ownership separate from idea `60` physical location
  ownership.

## Proof
- Not run. Lifecycle activation only.
