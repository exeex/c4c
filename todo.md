Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reactivation Prerequisite Mapping

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: mapped the remaining AArch64 call
move/source/preservation helpers against existing prepared call-plan facts.

Exact helper-to-prepared-fact mapping:

- `lower_before_call_moves` / `before_call_boundary_effects`:
  `prepare::plan_prepared_call_boundary_effects(call_plan, &bundle, nullptr)`
  already supplies ordered `PreparedCallBoundaryEffectPlan` entries for
  explicit `BeforeCall` moves and `PreservationHomePopulation` effects. This
  is the shared prepared ordering fact for before-call emission.
- `lower_after_call_moves` / `after_call_explicit_boundary_effects`:
  `prepare::plan_prepared_call_boundary_effects(call_plan, nullptr, &bundle)`
  supplies explicit `AfterCall` move effects, while the same planner supplies
  `PreservationRepublication` effects. This is the shared prepared ordering
  fact for after-call emission.
- `lower_before_call_move` argument matching: `classify_prepared_call_boundary_move`
  maps a `PreparedMoveResolution` to `PreparedCallArgumentPlan`,
  `PreparedAbiBinding`, or `PreparedCallResultPlan`; the argument-side prepared
  facts are `source_encoding`, `source_value_id`, `source_base_value_id`,
  `source_literal`, `source_symbol_name(_id)`, `source_register_name`,
  `source_slot_id`, `source_stack_offset_bytes`, `source_register_bank`,
  `source_base_value_name`, `source_pointer_byte_delta`,
  `source_register_placement`, and the destination register/stack fields.
- `make_frame_slot_call_argument_source`: the duplicated source facts are
  `PreparedCallArgumentPlan::source_encoding == FrameSlot`, `source_value_id`,
  `source_slot_id`, `source_stack_offset_bytes`, plus the value-home facts
  already captured by prepared value locations. Addressing-specific fallback
  still belongs to AArch64 emission until a more precise shared source-address
  fact exists.
- `make_frame_slot_call_argument_address_source` and
  `make_local_frame_address_call_argument_source`: partially mapped to
  `PreparedCallArgumentPlan::allows_local_aggregate_address_publication`,
  `source_encoding`, source value identity, and prepared address
  materializations. The object-name fallback for local aggregate address
  publication is not fully represented as a single call-argument source fact,
  so this is not the next cleanup target.
- byval register-lane paths (`prepared_byval_lane_extent_bytes`,
  `prepared_indirect_byval_extent_bytes`,
  `make_byval_register_lane_prepared_source`,
  `collect_byval_register_lane_stores`): partially mapped to
  `PreparedCallArgumentPlan` source/destination facts and prepared value homes,
  but lane-store materialization remains AArch64 emission logic. Not a clean
  deletion target for the next slice.
- `find_prior_preserved_value_for_call_argument` and
  `find_prior_preserved_value_for_value`: map to
  `PreparedCallPlanLookups::prior_preserved_by_value` through
  `find_latest_indexed_prior_preserved_value` and
  `find_dominating_indexed_prior_preserved_value`.
- `find_prior_stack_preserved_value_before_instruction` and
  `make_prior_stack_preserved_value_source`: map to
  `find_latest_indexed_prior_stack_preserved_value_before_instruction` and the
  `PreparedCallPreservedValue` stack-slot fields (`slot_id`,
  `stack_offset_bytes`, `stack_size_bytes`, `stack_align_bytes`).
- `make_prior_preserved_call_argument_source`: maps to
  `PreparedCallPreservedValue` route-specific source facts. Register sources
  use `route == CalleeSavedRegister`, `register_name`, `register_bank`,
  `register_placement`, `contiguous_width`, and `occupied_register_names`;
  stack sources use `route == StackSlot`, `slot_id`, `stack_offset_bytes`,
  `stack_size_bytes`, and `stack_align_bytes`.
- `make_callee_saved_preservation_home_population` and
  `make_callee_saved_preservation_home_republication_instruction`: map cleanly
  to `PreparedCallBoundaryEffectPlan` entries with effect kinds
  `PreservationHomePopulation` and `PreservationRepublication`; their source
  and destination endpoints are produced from `PreparedCallPreservedValue` by
  `append_preservation_call_boundary_effects`.

Valid next consolidation route: retire AArch64-local preservation selection
and effect-shaping helpers first, because prepared `preserved_values`,
call-plan lookup indexes, and preservation boundary effects already own the
decision. Do not target local-frame address or byval lane helper deletion until
the missing source-address/lane materialization fact is introduced.

## Suggested Next

Execute Step 2 against the preservation boundary: keep AArch64 code responsible
for emitting machine instructions, but replace local preservation selection and
synthetic effect shaping with direct consumption of prepared preservation
boundary effects.

## Watchouts

- Local-frame address publication and byval register-lane helpers are only
  partially mapped. They still depend on AArch64 materialization details and
  should not be used as the next deletion target under this runbook.
- Do not invent a new call-plan API under this source idea; the actionable route
  is the already-present preservation boundary facts.
- Do not claim progress through file concatenation, expectation weakening, or
  hidden helper rewrites.

## Proof

Ran delegated proof successfully:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: build passed; backend CTest subset passed, 162/162 tests.

Proof log: `test_after.log`.
