Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Save, Clobber, And Preservation Authority
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Completed the next Step 3.2 "Save, Clobber, And Preservation Authority"
packet for idea 88 by publishing call-clobber authority on the prepared
function-summary surface instead of forcing readers down into only the
detailed `call_plans` section.

Current packet result:
- prepared summary `callsite ...` lines now emit `clobbers=` alongside
  `preserves=` so Step 3.2 exposes both preservation and call-clobber facts in
  the same review surface
- summary clobber entries publish bank, register, contiguous width, and the
  occupied register-unit set in one compact authority string
- focused Step 3.2 coverage now proves the emitted summary string is derived
  from prepared call-plan clobber authority instead of a separate printer-only
  guess

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
- Treat the summary clobber string as a publication of prepared authority, not
  as permission to infer or invent additional target-specific call behavior.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
