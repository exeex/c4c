Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove Or Narrow Duplicate Move Decision Logic

# Current Packet

## Just Finished

Step 2 inspection found no remaining narrow mixed lookup/emission target after
the prior-preserved call-argument split, prior stack-preserved value split, and
callee-saved home population suppression split.

`calls.hpp` now exposes prepared lookup wrappers separately from the narrowed
operand/emission helpers. In `calls_moves.cpp`, the prior-preserved
call-argument path and prior stack-preserved value path call lookup helpers
first, then pass selected prepared facts into AArch64 operand construction; the
callee-saved home population suppression check now happens in the
`lower_before_call_moves` effect loop before emission. In
`calls_preservation.cpp`, the remaining preservation helpers consume prepared
effect facts and perform only operand-construction value-home lookup, not
duplicate prior-preservation selection.

## Suggested Next

Step 2 is exhausted. Suggested next packet: advance to Step 3 consolidation of
the remaining `calls.hpp` export surface, focusing on which helpers still need
to be exported versus made file-local after the lookup/emission boundary splits.

Suggested Step 3 owned files: `src/backend/mir/aarch64/codegen/calls.hpp`,
`src/backend/mir/aarch64/codegen/calls_moves.cpp`,
`src/backend/mir/aarch64/codegen/calls_preservation.cpp`, `todo.md`, and
`test_after.log`.

Suggested focused proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_runtime_byval_helper_payload_8_to_13|backend_runtime_byval_helper_payload_9_to_14|backend_runtime_byval_helper_mixed_hfa_payload)$' > test_after.log 2>&1`

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
- `make_callee_saved_preservation_home_population` should stay an emission
  helper for already-eligible prepared effects; prior-preserved suppression now
  belongs in the `lower_before_call_moves` population-effect loop.
- Remaining source selection inside `lower_before_call_move` is tied to the
  missing prepared call-argument source fact noted above, so do not treat it as
  another narrow Step 2 target unless that prepared fact exists.

## Proof

Inspection-only packet: no build/tests were run and logs were not modified.
