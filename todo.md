Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove Or Narrow Duplicate Move Decision Logic

# Current Packet

## Just Finished

Step 2 split the prior stack-preserved value source path in
`lower_value_moves`. `find_prior_stack_preserved_value_before_instruction`
remains the prepared lookup/eligibility owner, while the new file-local
`make_prior_stack_preserved_value_source` helper consumes the selected
`PreparedCallPreservedValue` and constructs the AArch64 `MemoryOperand`.

The BeforeInstruction value-move branch still calls the lookup first, then
passes the selected preserved value to the emission helper. Ordinary
`source_home` register and stack-slot fallback logic remains in
`lower_value_moves`, with no move ordering changes.

## Suggested Next

Next packet should inspect whether Step 2 is now exhausted after the
call-argument prior-preserved split and the value-move prior stack-preserved
split. If no narrow mixed lookup/emission target remains, suggest Step 3
consolidation.

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
- Do not use this target to refactor `lower_value_moves` wholesale, merge
  BlockEntry republication with BeforeInstruction lowering, or change move
  ordering.
- `find_prior_stack_preserved_value_before_instruction` still owns prior
  stack-preservation lookup and stack-route eligibility. The new
  `make_prior_stack_preserved_value_source` helper should stay operand
  construction only.

## Proof

Proof passed with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_runtime_byval_helper_payload_8_to_13|backend_runtime_byval_helper_payload_9_to_14|backend_runtime_byval_helper_mixed_hfa_payload)$' > test_after.log 2>&1`

`test_after.log` records 10 selected tests passing after the build.
