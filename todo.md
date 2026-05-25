Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map The Call Move And Preservation Boundary

# Current Packet

## Just Finished

Step 1 mapped the exported AArch64 call move and preservation boundary in
`calls.hpp` against the current calls-family implementation.

Ownership map:

- `lower_before_call_moves`: exported dispatch entry for BeforeCall move
  emission. It consumes a prepared move bundle plus
  `plan_prepared_call_boundary_effects`; it still owns target-local selection
  inside `lower_before_call_move` for register, immediate, prior-preserved,
  byval-lane, local-frame-address, aggregate, and stack-destination sources.
- `lower_after_call_moves`: exported dispatch entry for AfterCall result moves
  and callee-saved republication. It is mostly emission from prepared bundle and
  prepared boundary-effect facts.
- `lower_before_return_moves`: exported return-ABI move emission from a prepared
  BeforeReturn bundle. This is emission-only except for value-home lookup needed
  to build AArch64 operands.
- `lower_value_moves`: exported generic value-move emission for BlockEntry and
  BeforeInstruction. It also consumes indexed block-entry republication effects;
  the move lowering is emission-oriented, but prior stack-preservation lookup
  remains a prepared-fact lookup path.
- `call_boundary_move_reloads_materialized_address` and
  `order_before_call_moves_for_source_preservation`: exported dispatch-only
  machine-record helpers. They do not rederive call-plan facts, but they are
  emission-ordering helpers rather than durable prepared authority.
- `find_move_bundle`: exported lookup wrapper around indexed prepared move
  bundles. It is prepared-fact lookup ownership, not emission.
- `find_prior_preserved_value_for_call_argument` and
  `find_prior_preserved_value_for_value`: exported prepared preservation lookup
  wrappers. They should stay lookup-only unless a direct prepared source fact
  removes the need for target-local querying.
- `make_prior_preserved_call_argument_source`: mixed ownership. It queries prior
  preserved values, suppresses the path for local-frame address and byval cases,
  and creates AArch64 operands. This is the clearest duplicate prepared-fact
  boundary because `lower_before_call_move` asks it to discover whether the
  call-argument source should come from prior preservation.
- `make_callee_saved_preservation_home_republication_instruction`,
  `make_callee_saved_preservation_home_republication`, and
  `make_callee_saved_preservation_home_population`: emission from prepared
  boundary-effect facts. `make_callee_saved_preservation_home_population` still
  checks prior preservation before emitting, but the primary selection is
  already driven by `PreparedCallBoundaryEffectPlan`.
- `make_sret_memory_return_address_source`,
  `make_frame_slot_call_argument_source`,
  `make_frame_slot_call_argument_address_source`,
  `make_local_frame_address_call_argument_source`,
  `make_stack_call_argument_destination`, and
  `make_aggregate_call_argument_source`: AArch64 operand construction from
  prepared value-home, addressing, ABI binding, and stack-layout facts. These
  are emission/source-operand factories, with no immediate code-changing target
  selected.
- `is_aarch64_byval_register_lane_move`,
  `collect_byval_register_lane_stores`,
  `make_byval_register_lane_prepared_source`,
  `aggregate_lane_store_memory`, and
  `prepared_indirect_byval_extent_bytes`: AArch64 byval-lane and aggregate
  source helpers. They remain target-local emission helpers, but byval extent
  and lane-source reconstruction are blockers for moving all source selection
  out of `lower_before_call_move`.

## Suggested Next

Next code-changing target: split the prior-preserved call-argument source path
so the lookup and emission responsibilities are no longer bundled in
`make_prior_preserved_call_argument_source`.

Narrow packet shape:

- Keep `find_prior_preserved_value_for_call_argument` as the prepared lookup
  owner.
- Narrow `make_prior_preserved_call_argument_source` into an emission-only helper
  that consumes an already-selected `PreparedCallPreservedValue`.
- Leave byval and local-frame address exclusion in the caller until a direct
  prepared call-argument source fact exists.
- Update the `calls.hpp` declaration only for that helper boundary.

This remains inside the source idea because it removes mixed prepared-fact
lookup plus AArch64 operand emission ownership from the call move boundary.

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
- If the mapping proves no narrow calls-emission target remains, stop and send
  the lifecycle state back to plan-owner for closure or deactivation review.
- Missing prepared-fact blocker: there is no single prepared call-argument source
  fact that says a selected BeforeCall argument move should source from prior
  preservation versus local-frame address materialization versus byval
  register-lane materialization. Because of that, the next packet should split
  lookup from emission but should not try to remove all caller-side source
  selection.

## Proof

Mapping-only packet: no build or tests were run, and no `test_after.log` was
created.

Selected focused proof command for the next code-changing packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_runtime_byval_helper_payload_8_to_13|backend_runtime_byval_helper_payload_9_to_14|backend_runtime_byval_helper_mixed_hfa_payload)$' > test_after.log 2>&1`

I verified the regex with `ctest --test-dir build -N -R ...`; it selects 12
tests covering AArch64 call moves, call-boundary moves, preserved call
arguments, byval aggregate arguments, and prepared preservation effects.
