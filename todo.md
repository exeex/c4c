Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3.1.3
Current Step Title: Result Source Authority Completion
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed Step 3.1.3 "Result Source Authority Completion" packet work for
idea 88 by publishing explicit result-side ABI source storage shape in
`PreparedCallResultPlan`, threading the authoritative source storage kind and
stack-offset carrier facts through prepared call-plan population, and
extending prepared dumps plus focused contract/printer coverage so consumers do
not infer register-shaped scalar call-result carriers solely from the
incidental presence of `source_reg`.

Current packet result:
- `PreparedCallResultPlan` now carries `source_storage_kind` and optional
  `source_stack_offset_bytes` alongside the existing ABI carrier register and
  bank fields.
- `populate_call_plans` now copies the authoritative after-call ABI binding
  storage shape directly into prepared result plans instead of leaving
  consumers to infer register shape from `source_reg`.
- Prepared callsite summaries and detailed call-plan dumps now print scalar
  result source storage explicitly, including register-vs-stack source shape.
- Backend contract and printer tests now prove spilled scalar call results
  still publish a direct register-shaped ABI source while retaining their
  frame-slot destination home.

## Suggested Next

Continue Step 3.1.3 "Result Source Authority Completion" by auditing whether
scalar prepared call results still need a direct source identity publication
that does not force consumers to recover the semantic owner only through
`destination_value_id` or instruction correlation, then publish that next
result-side identity fact directly in `PreparedCallPlan`.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep call-boundary authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- Treat result source shape and result source identity as separate facts:
  `source_storage_kind` now answers "what ABI carrier shape produces the
  scalar result," while any future identity follow-up should answer "which
  prepared scalar or carrier owner that result semantically comes from."
- Result-side follow-up should stay inside scalar prepared call authority; do
  not widen into grouped-register tuple publication or target-specific ABI
  reconstruction.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
