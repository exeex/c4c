Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Save, Clobber, And Preservation Authority
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed Step 3.2 "Save, Clobber, And Preservation Authority" packet work
for idea 88 by auditing the current prepared call clobber publication and
confirming the next real scalar gap was per-call preservation authority:
`clobbered_registers` already published the ABI destroy set, but `call_plans`
still did not publish which scalar prepared values actually survive a given
call boundary and by what preservation route. Added direct per-call preserved
scalar publication for live-across-call scalar values.

Current packet result:
- `PreparedCallPlan` now carries `preserved_values`, a direct prepared list of
  scalar values that remain live across that specific call boundary.
- Each preserved scalar now publishes its value identity plus the preservation
  route currently keeping it safe across the call: callee-saved register,
  stack slot, or `unknown` when the packet cannot yet classify a more precise
  route.
- `populate_call_plans` now derives preserved scalar entries from existing
  liveness/regalloc cross-call facts instead of leaving consumers to correlate
  `call_plans` against regalloc and live-interval state manually.
- Prepared summaries and detailed call-plan dumps now expose preserved scalar
  values in a reviewable form.
- Focused contract and printer coverage now prove a single call-crossing
  scalar carry value is published directly with its callee-saved GPR
  preservation route.

## Suggested Next

Continue Step 3.2 by auditing whether scalar stack-preserved and
unclassified-route cases need more explicit route detail than the current
`preserved_values` publication, or whether the next honest packet is broader
save/restore ownership linkage between per-call preserved values and
function-level saved-callee-register publication.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep call-boundary authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- Keep Step 3.2 focused on scalar preservation facts already known to prepared
  liveness/regalloc; do not widen into grouped-register preservation routes.
- Treat `clobbered_registers` and `preserved_values` as complementary facts:
  one publishes what the call may destroy, the other publishes which scalar
  prepared values survive that boundary and the current route keeping them
  safe.
- `unknown` preservation routes are honest bounded publication, not a license
  to guess target-specific save/restore policy in later consumers.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
