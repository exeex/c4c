Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate The Remaining Export Surface

# Current Packet

## Just Finished

Step 3 consolidated one remaining calls move/preservation export. The
`make_callee_saved_preservation_home_republication` wrapper is now file-local
to `calls_moves.cpp`, its `calls.hpp` declaration was removed, and the old
cross-translation-unit wrapper definition was removed from
`calls_preservation.cpp`.

Remaining calls move/preservation declarations in `calls.hpp` still have
cross-translation-unit callers:

- `lower_before_call_moves`, `lower_after_call_moves`,
  `lower_before_return_moves`, and `lower_value_moves` are dispatch/test entry
  points.
- `call_boundary_move_reloads_materialized_address` and
  `order_before_call_moves_for_source_preservation` are used by dispatch after
  BeforeCall lowering.
- `find_move_bundle` is used by `calls_dispatch_bridge.cpp` and
  `calls_moves.cpp`.
- `find_prior_preserved_value_for_call_argument` and
  `find_prior_preserved_value_for_value` are defined in
  `calls_preservation.cpp` and used from `calls_moves.cpp`.
- `make_prior_preserved_call_argument_source` is defined outside the owned
  files in `calls_argument_sources.cpp` and used from `calls_moves.cpp`.
- `make_callee_saved_preservation_home_republication_instruction` and
  `make_callee_saved_preservation_home_population` are defined in
  `calls_preservation.cpp` and used from `calls_moves.cpp`.

## Suggested Next

Next packet should advance to Step 4 broader backend checkpoint validation, or
route to plan-owner if the supervisor wants a lifecycle review before broader
validation.

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
- Do not remove the remaining calls move/preservation declarations from
  `calls.hpp` without first moving their definitions or callers into the same
  translation unit; the current remaining declarations all have cross-TU uses.

## Proof

Proof passed with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_runtime_byval_helper_payload_8_to_13|backend_runtime_byval_helper_payload_9_to_14|backend_runtime_byval_helper_mixed_hfa_payload)$' > test_after.log 2>&1`

`test_after.log` records 10 selected tests passing after the build.
