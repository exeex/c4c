Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Save, Clobber, And Preservation Authority
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Completed the next Step 3.2 "Save, Clobber, And Preservation Authority"
packet for idea 88 by auditing non-callee-saved scalar preservation routes and
repairing the stack-preserved publication gap in `call_plans`.

Current packet result:
- stack-preserved preserved values no longer publish raw regalloc spill-slot
  coordinates; `build_call_preserved_values` now prefers prepared value-home
  or stack-object-backed frame-slot authority before falling back to internal
  assigned-slot data
- focused Step 3.2 coverage now proves a cross-call scalar spilled under
  pressure publishes the same frame-slot identity and stack offset through
  both `call_plans` and `storage_plans`
- the same audit did not uncover additional direct prepared authority for
  current `unknown` routes in focused fixtures, so that route remains an
  honest bounded publication rather than guessed save/restore policy

## Suggested Next

Continue Step 3.2 by checking whether any remaining scalar call-clobber or
preservation facts still force backend consumers to correlate `call_plans`
against lower-level prepared tables instead of following one direct authority
record.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep call-boundary authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- Keep Step 3.2 focused on scalar preservation facts already known to prepared
  frame/regalloc state; do not widen into grouped-register preservation routes.
- Treat stack-preserved authority as prepared frame-slot publication, not as
  permission to leak raw regalloc spill-slot numbering to consumers.
- `unknown` preservation routes are still honest bounded publication, not a
  license to guess target-specific save/restore policy in later consumers.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
