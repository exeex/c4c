Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Save, Clobber, And Preservation Authority
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Completed the next Step 3.2 "Save, Clobber, And Preservation Authority"
packet for idea 88 by publishing stack-preserved frame-slot identity on the
prepared function-summary surface instead of forcing readers down into only
the detailed `call_plans` records.

Current packet result:
- stack-preserved `preserves=` summary entries now publish the prepared
  frame-slot id alongside the stack offset so consumers can follow one direct
  callsite summary surface for stack-preserved scalars
- focused Step 3.2 contract and printer coverage now prove the summary string
  is derived from existing prepared preservation authority rather than a
  printer-only guess
- register-preserved summary and detail surfaces remain unchanged, so this
  packet closes the remaining stack-slot summary gap without widening Step 3.2
  into grouped-register work

## Suggested Next

Continue Step 3.2 by checking whether any remaining scalar call-boundary facts
still require consumers to correlate summary lines against detailed call-plan
records instead of following one direct prepared authority surface.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep call-boundary authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- Keep Step 3.2 focused on scalar preservation and clobber facts already known
  to prepared frame/regalloc state; do not widen into grouped-register routes.
- Keep the summary surface compact and reviewable; do not duplicate the full
  detailed call-plan payload into the summary line.
- Treat the new stack-slot summary token as a publication of prepared
  frame-slot authority, not as permission to infer additional save/restore
  policy beyond the published prepared facts.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer)$'`.
Result: pass.
Proof log: `test_after.log`.
