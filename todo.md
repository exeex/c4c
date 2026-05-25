Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove Or Narrow Duplicate Move Decision Logic

# Current Packet

## Just Finished

Step 2 inspection found one remaining narrow mixed lookup/emission target after
the prior-preserved call-argument split and prior stack-preserved value split.

`calls.hpp` now exposes lookup-only preservation wrappers plus the narrowed
`make_prior_preserved_call_argument_source` operand helper. In
`calls_moves.cpp`, prior-preserved call-argument and prior stack-preserved
value paths already call lookup first and pass selected prepared facts into
emission helpers. The remaining mixed boundary is in
`calls_preservation.cpp`: `make_callee_saved_preservation_home_population`
consumes a prepared `PreservationHomePopulation` effect and emits the AArch64
machine instruction, but also calls `find_prior_preserved_value_for_value` to
suppress population when a prior preserved value already dominates.

## Suggested Next

Next code-changing target: split the callee-saved preservation home population
prior-preserved suppression from AArch64 instruction emission.

Narrow packet shape:

- Keep `find_prior_preserved_value_for_value` as the prepared lookup owner.
- Move the prior-preserved suppression decision out of
  `make_callee_saved_preservation_home_population`, likely into the
  `lower_before_call_moves` loop that already iterates
  `PreservationHomePopulation` effects.
- Leave `make_callee_saved_preservation_home_population` as an emission helper
  that consumes an already-eligible prepared effect and builds the AArch64
  instruction.
- Keep value-home lookup inside the emission helper because it is operand
  construction input, not prepared preservation selection.
- Do not change effect ordering, explicit move lowering, or republication
  handling.

Owned files should be
`src/backend/mir/aarch64/codegen/calls_preservation.cpp`,
`src/backend/mir/aarch64/codegen/calls_moves.cpp`, `todo.md`, and
`test_after.log`; include `src/backend/mir/aarch64/codegen/calls.hpp` only if a
narrow helper signature/declaration change is required.

This remains inside the source idea because it removes one more target-local
prepared-preservation lookup from an AArch64 emission helper while preserving
prepared effect planning as the source of the population effect.

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
- For the remaining population target, do not move republication logic or
  change `PreparedCallBoundaryEffectPlan` semantics. The intended split is only
  the prior-preserved suppression lookup versus population instruction emission.

## Proof

Inspection-only packet: no build/tests were run and logs were not modified.

Focused proof command for the next code-changing packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_runtime_byval_helper_payload_8_to_13|backend_runtime_byval_helper_payload_9_to_14|backend_runtime_byval_helper_mixed_hfa_payload)$' > test_after.log 2>&1`
