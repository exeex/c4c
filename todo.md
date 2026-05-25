Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove Or Narrow Duplicate Move Decision Logic

# Current Packet

## Just Finished

Step 2 split the prior-preserved call-argument source boundary.
`lower_before_call_move` now keeps the local-frame address and byval exclusions
in the caller, invokes `find_prior_preserved_value_for_call_argument` as the
prepared lookup owner, and passes the selected
`PreparedCallPreservedValue` into the narrowed
`make_prior_preserved_call_argument_source` helper. The exported helper
declaration now consumes the selected preserved value and constructs only the
AArch64 source operand.

## Suggested Next

Next packet should inspect the remaining calls preservation helpers for another
narrow mixed lookup/emission boundary, or send the lifecycle state back to
plan-owner if this was the last Step 2 target.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not broaden this checkpoint into whole dispatch cleanup or a whole
  calls-family rewrite.
- Do not claim helper renames, expectation rewrites, or testcase-shaped
  shortcuts as progress.
- Keep AArch64-specific register, memory, frame-slot, byval lane, immediate,
  constant-materialization, and assembly spelling in AArch64 code.
- Keep source-idea progress tied to deleted duplication, moved ownership, or a
  sharper emission-only boundary.
- Do not revisit completed call printing/effect-publication routes unless this
  mapping proves one still owns duplicate prepared-fact authority.
- The old mixed `make_prior_preserved_call_argument_source` overload body was
  removed after confirming the active declaration and call site use the
  narrowed helper that consumes `PreparedCallPreservedValue`.
- Missing prepared-fact blocker remains: there is no single prepared
  call-argument source fact that says a selected BeforeCall argument move should
  source from prior preservation versus local-frame address materialization
  versus byval register-lane materialization. Caller-side source selection is
  therefore still intentionally present.

## Proof

Proof passed with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_runtime_byval_helper_payload_8_to_13|backend_runtime_byval_helper_payload_9_to_14|backend_runtime_byval_helper_mixed_hfa_payload)$' > test_after.log 2>&1`

`test_after.log` records 10 selected tests passing after the build.
