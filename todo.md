Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Call Boundary Authority Completion
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed Plan Step 3 "Call Boundary Authority Completion" for idea 88 by
publishing explicit variadic floating-register argument count in
`PreparedCallPlan`, printing it in prepared dumps, and tightening
backend/prealloc tests so downstream x86 consumers no longer need to recover
the `%al`-relevant scalar FPR count from raw call ABI metadata.

Current packet result:
- `PreparedCallPlan` now publishes `variadic_fpr_arg_register_count` alongside
  existing wrapper and callee identity facts.
- `populate_call_plans` now counts scalar SSE/FPR arguments passed in registers
  for variadic calls instead of leaving that call-boundary fact implicit in
  raw `arg_abi` entries.
- Prepared dumps now expose the published variadic FPR count in both the
  summary callsite view and the `prepared-call-plans` detail section.
- Backend/prealloc tests now prove a direct variadic extern call with one
  floating-register argument publishes `variadic_fpr_arg_register_count=1`
  while non-variadic and indirect wrapper cases stay at zero.

## Suggested Next

Continue Step 3 by auditing whether any remaining scalar variadic call facts
beyond register-count publication still require target-local recovery, with the
best next packet likely focused on indirect or memory-return call-boundary
authority if x86 consumers still cannot follow prepared plans directly.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- The new count is intentionally scalar-only: it tracks SSE/FPR arguments that
  BIR already classifies as register-passed and should not be stretched into
  grouped-register semantics owned by idea 89.
- Do not expand this runbook into target-specific instruction spelling or
  generic x86 behavior recovery; this packet stayed at the prepared contract
  boundary only.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
