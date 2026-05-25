Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove Or Narrow Duplicate Move Decision Logic

# Current Packet

## Just Finished

Step 2 follow-up inspection found one remaining narrow mixed
lookup/emission target in `calls_moves.cpp`: the BeforeInstruction value-move
path uses `find_prior_stack_preserved_value_before_instruction` to perform a
prepared prior-stack preservation lookup and stack-route validation, then
constructs the AArch64 `MemoryOperand` inline in `lower_value_moves`.

The callee-saved republication helpers in `calls_preservation.cpp` are already
mostly effect-driven emission from `PreparedCallBoundaryEffectPlan`. They still
perform value-home lookup needed to build operands, but the selected
preservation/republication effect comes from prepared effect planning. The
remaining Step 2 target is therefore the prior stack-preserved value source path
inside value-move lowering, not another call-argument helper.

## Suggested Next

Next code-changing target: split the prior stack-preserved value source path in
`lower_value_moves`.

Narrow packet shape:

- Keep `find_prior_stack_preserved_value_before_instruction` as the prepared
  lookup/eligibility owner for prior stack-preserved values.
- Add a small emission-only helper that consumes an already-selected
  `PreparedCallPreservedValue` and constructs the corresponding AArch64
  `MemoryOperand`.
- Update the `lower_value_moves` BeforeInstruction branch to call the lookup
  first and pass the selected preserved value to the operand helper.
- Keep ordinary `source_home` register/stack-slot fallback logic in
  `lower_value_moves`; do not broaden this into value-move dispatch cleanup.

Owned files for the next packet should be
`src/backend/mir/aarch64/codegen/calls_moves.cpp` and `todo.md`. Add
`src/backend/mir/aarch64/codegen/calls.hpp` only if the helper needs to be
exported; it can likely remain file-local.

This remains inside the source idea because it removes another target-local
prepared-fact decision plus AArch64 operand construction bundle from the calls
move lowering boundary.

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
- Do not move or rewrite `find_prior_stack_preserved_value_before_instruction`
  unless the next packet explicitly owns that lookup boundary; the immediate
  split can preserve it as the lookup owner and only extract the memory operand
  construction.
- Do not use this target to refactor `lower_value_moves` wholesale, merge
  BlockEntry republication with BeforeInstruction lowering, or change move
  ordering.

## Proof

Inspection-only packet: no build/tests were run and `test_after.log` was not
modified.

Focused proof command for the next code-changing packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_runtime_byval_helper_payload_8_to_13|backend_runtime_byval_helper_payload_9_to_14|backend_runtime_byval_helper_mixed_hfa_payload)$' > test_after.log 2>&1`
