Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Argument And Result Source Authority
Plan Review Counter: 6 / 6
# Current Packet

## Just Finished

Completed Step 3.1 "Argument And Result Source Authority" packet work for idea
88 by publishing authoritative prepared scalar identity for computed-address
argument bases in `PreparedCallPlan`, printing that base identity in prepared
dumps, and tightening focused call-argument source-shape coverage so consumers
do not have to recover the computed-address base scalar id from storage plans
or value-location correlation.

Current packet result:
- `PreparedCallArgumentPlan` now carries `source_base_value_id` alongside the
  existing computed-address base name and byte delta authority.
- `populate_call_plans` now resolves that base id from prepared value homes,
  with regalloc-name fallback when the base home is not directly available.
- Prepared callsite summaries and detailed call-plan dumps now print the
  computed-address base scalar id directly.
- Backend contract and printer tests now prove computed-address argument
  publication includes the base scalar id directly in `PreparedCallPlan`.

## Suggested Next

Continue Step 3.1 by checking whether any remaining scalar call-boundary
result-side fact still requires consumers to correlate prepared call plans with
move bundles indirectly, then publish that next missing result source-shape or
identity fact directly in `PreparedCallPlan`.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep call-boundary authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- Treat computed-address shape and computed-address base identity as separate
  facts: `source_base_value_id` answers "which prepared scalar owns the base,"
  while base name and delta answer "how the address is formed."
- Result-side follow-up should stay inside scalar prepared call authority; do
  not widen into grouped-register tuple publication or target-specific ABI
  reconstruction.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
