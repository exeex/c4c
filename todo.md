Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Call Boundary Authority Completion
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Completed Plan Step 3 "Call Boundary Authority Completion" for idea 88 by
publishing explicit scalar wrapper-kind classification in `PreparedCallPlan`,
printing it in prepared dumps, and tightening direct backend/prealloc tests so
x86 consumers no longer need to infer wrapper shape from semantic-module state.

Current packet result:
- `PreparedCallPlan` now publishes `wrapper_kind` with explicit
  `same_module`, `direct_extern_fixed_arity`, `direct_extern_variadic`, and
  `indirect` values while preserving existing `is_indirect` and
  `direct_callee_name`.
- `populate_call_plans` now classifies wrapper shape from the prepared module’s
  direct callee surface instead of leaving x86-facing consumers to reconstruct
  it from semantic-module state.
- Prepared dumps now expose wrapper-kind authority in both the summary
  callsite view and the `prepared-call-plans` detail section.
- Backend/prealloc tests now assert all four wrapper kinds through prepared
  call plans and prepared printer output.

## Suggested Next

Continue Step 3 by auditing the next scalar call-boundary fact that x86 still
has to recover outside the prepared contract. The likely next packet is
variadic-call scalar metadata such as `%al` floating-argument count if that is
still reconstructed backend-locally.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- The wrapper classifier assumes any defined direct callee is `same_module` and
  any declaration-only direct callee is extern. If BIR grows more direct-call
  surface states later, keep the prepared classification module-authoritative
  instead of drifting back into target-local inference.
- Do not expand this runbook into target-specific instruction spelling or
  generic x86 behavior recovery; the x86 module emitter is still intentionally
  stubbed, so this packet stayed at the contract boundary only.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
