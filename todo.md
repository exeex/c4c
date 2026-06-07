# Current Packet

Status: Active
Source Idea Path: ideas/open/121_aarch64_calls_preservation_republication_visibility_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Preserve-Effect Publication Visibility

## Just Finished

Step 1: Characterize Existing Preservation Visibility is complete.

Cluster map recorded from existing code and tests:

- Preserve-effect publication on call nodes is gated in `calls.cpp` by
  `prepared_call_preserve_effect_publication_enabled()`: enabled mode copies
  `call_plan.preserved_values` into `CallInstructionRecord::preserved_values`
  and into `InstructionRecord::preserves`; disabled mode emits an empty
  preserved-value payload/effect list. `asm_emitter.cpp` disables this through
  `ScopedPreparedCallPreserveEffectPublication(false)`.
- Shared/prepared planning owns the preservation route facts in
  `plan_prepared_call_boundary_effects`: each non-unknown
  `PreparedCallPreservedValue` contributes a before-call
  `PreservationHomePopulation` effect and an after-call
  `PreservationRepublication` effect, carrying source/destination endpoints,
  route, storage kind, register/span data, stack slot/offset/size/alignment,
  placement, and reason.
- AArch64 before-call preservation population is consumed in
  `lower_before_call_moves` through
  `make_callee_saved_preservation_home_population`, which emits only
  callee-saved-register population records from prepared endpoints.
- Prior-preserved call arguments are consumed in
  `make_prior_preserved_call_argument_source` for explicit
  `PriorPreservation` source selections, with callee-saved-register and
  stack-slot routes failing closed when prepared selection fields are
  incomplete.
- Prior stack-preserved value fallback for before-instruction moves is consumed
  by `find_prior_stack_preserved_value_before_instruction` and
  `StackFrameSlotCallOperandOwner::prior_stack_preserved_value_source`, using
  indexed prior stack-preserved lookup plus prepared endpoint stack storage.
- After-call and block-entry republication are emitted through
  `make_callee_saved_preservation_home_republication_instruction`, driven by
  `plan_prepared_call_boundary_effects` after-call effects and
  `indexed_block_entry_republication_effects_for_block`.
- Stack-preserved first-call publication is handled by
  `publish_stack_preserved_call_values`, which consumes
  `first_indexed_stack_preserved_values_for_call`, current emitted scalar
  register state, and prepared current value homes before emitting frame-slot
  stores.

Existing visibility surfaces:

- `backend_call_boundary_effect_plan` already exposes shared prepared
  preservation intent for both callee-saved and stack-slot routes, including
  before-call population, after-call republication, endpoint storage, register
  placement, stack slot/offset/size/alignment, and spill placement.
- `backend_aarch64_target_instruction_records` exposes enabled preserve-effect
  publication on AArch64 machine call nodes for callee-saved and stack-slot
  preserved values, including prepared register and memory operands. It also
  has fail-closed coverage for incomplete stack-slot preserve extents.
- `backend_aarch64_call_boundary_owner` proves the current AArch64 lowering
  consumes shared boundary effects for callee-saved population and
  republication.
- `backend_aarch64_instruction_dispatch` has existing route tests for
  prior-preserved call arguments and preservation homes:
  `nested_call_argument_publishes_from_prior_preservation_home`,
  `selected_register_prior_preservation_publishes_from_preserved_home`,
  `incomplete_callee_saved_prior_preservation_selection_does_not_rederive_prior_home`,
  `cross_block_call_argument_publishes_from_prior_preservation_home`,
  `preservation_home_population_consumes_prepared_endpoints`,
  `preservation_home_population_does_not_rederive_self_alias_endpoint`,
  `preserved_home_feeds_later_non_call_scalar_after_clobber`,
  `stack_preserved_home_feeds_later_non_call_scalar_after_clobber`,
  `stack_preserved_home_feeds_later_call_argument_after_clobber`,
  `stack_preserved_caller_saved_home_feeds_later_second_call_argument`, and
  `incomplete_stack_prior_preservation_selection_does_not_rederive_prior_home`.
- Existing stack-preserved argument consumption surfaces also include
  `preserved_stack_home_symbol_address_argument_not_reloaded_after_materialization`
  and `stack_preserved_loaded_global_pointer_publishes_before_call_argument_reload`.
- Current stack-to-register republication visibility is partial:
  `after_call_result_publication_precedes_preservation_republication` proves an
  after-call result store precedes a callee-saved register republication
  (`x20 -> x1`), while `backend_call_boundary_effect_plan` exposes shared
  callee-saved and stack-slot republication intent.

Visibility gaps for later packets:

- Preserve-effect publication enabled is visible through
  `backend_aarch64_target_instruction_records`, but disabled publication is
  only implied by `asm_emitter.cpp` suppressing the scoped flag. There is no
  focused test that toggles disabled mode and proves preserved payload/effects
  are absent while the prepared call still owns `preserved_values`.
- Prior stack-preserved argument consumption exists in dispatch tests, but a
  narrow Step 3 packet should make sure the route explicitly distinguishes
  prepared prior-preservation selection from local stack-home reselection.
- Stack-to-register republication through preservation facts is visible for
  callee-saved register homes, but the current route-visible proof does not yet
  isolate a stack-slot preserved endpoint driving a later register
  republication record through the same preservation route.

Recommended focused proof subset for later code/test packets:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_call_boundary_effect_plan|backend_aarch64_target_instruction_records|backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch)$' >> test_after.log 2>&1`

## Suggested Next

Execute Step 2: add focused visibility for preserve-effect publication enabled
and disabled, preferably by extending the existing AArch64 target instruction
record surface so the disabled scoped-publication route is explicit without
changing preservation ownership.

## Watchouts

- Do not extract a preservation owner before the visibility contract exists.
- Do not reopen stack-preserved source, endpoint stack storage, or frame-slot
  ownership from idea 93.
- Do not move AArch64 callee-saved register spelling, frame-slot store lines,
  preservation-home publication records, or ABI/local effect resources into
  shared code.
- Treat expectation downgrades, unsupported-path rewrites, and helper-renaming
  claims without new route-visible evidence as route failures.
- The disabled preserve-effect route should prove the effect/payload publication
  toggle, not erase prepared `call_plan.preserved_values` or weaken existing
  enabled-mode record checks.

## Proof

No build or test command was required for this docs/analysis-only packet because
only `todo.md` changed. No `test_after.log` was generated.
