Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Argument And Result Source Authority
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Completed Step 3.1 "Argument And Result Source Authority" packet work for idea
88 by publishing authoritative prepared scalar identity for indirect callees in
`PreparedCallPlan`, printing that identity in prepared dumps, and tightening
focused indirect-call contract coverage so consumers do not have to recover the
callee's scalar id from storage plans or side-table lookups.

Current packet result:
- `PreparedIndirectCalleePlan` now carries `value_id` alongside the existing
  named/home-shape authority for indirect callees.
- `build_indirect_callee_plan` now preserves the prepared scalar `value_id`
  from the callee's published prepared home when value locations are available.
- Prepared callsite summaries and detailed call-plan dumps now print
  `indirect_value_id=...` for indirect callees.
- Backend contract and printer tests now prove indirect-call publication
  includes the callee's prepared scalar id directly in `PreparedCallPlan`.

## Suggested Next

Continue Step 3.1 by checking whether any remaining scalar call-boundary fact
still requires consumers to correlate prepared call plans with storage plans or
move bundles indirectly, then publish that next missing identity directly in
`PreparedCallPlan`.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep call-boundary authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- Treat prepared scalar identity and home shape as separate facts: `value_id`
  answers "which scalar," while register/slot/offset answers "where it lives."
- If a later packet needs identity publication when prepared value locations are
  absent, avoid inventing target-local recovery logic in call consumers.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
