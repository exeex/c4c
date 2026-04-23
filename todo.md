Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Save, Clobber, And Preservation Authority
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Completed Step 3.2 "Save, Clobber, And Preservation Authority" packet work
for idea 88 by wiring per-call callee-saved preservation entries directly to
the matching function-level save/restore authority instead of leaving backend
consumers to correlate `call_plans` against `saved_callee_registers` by
register name alone.

Current packet result:
- `PreparedCallPreservedValue` now publishes `callee_saved_save_index` when a
  preserved scalar survives the call in a callee-saved register.
- `populate_call_plans` now resolves that index against the function's
  published `saved_callee_registers`, so the call-plan entry points at the
  same save/restore authority record that owns the prologue/epilogue work.
- Prepared summaries and detailed call-plan dumps now expose that save
  ownership directly as part of the preserved-value publication.
- Focused contract and printer coverage now prove the call-crossing `carry`
  value references the matching frame-plan save authority for `s1`.

## Suggested Next

Continue Step 3.2 by auditing whether stack-preserved or currently `unknown`
cross-call scalar values need similarly direct function-level preservation
ownership, or whether the remaining honest work shifts to another explicit
save/restore authority gap within the same step.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep call-boundary authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- Keep Step 3.2 focused on scalar preservation facts already known to prepared
  frame/regalloc state; do not widen into grouped-register preservation routes.
- Treat `callee_saved_save_index` as a reference into function-local frame-plan
  authority, not as a replacement for the existing published register/bank
  facts.
- `unknown` preservation routes are still honest bounded publication, not a
  license to guess target-specific save/restore policy in later consumers.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
