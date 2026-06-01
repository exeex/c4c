Status: Active
Source Idea Path: ideas/open/76_aarch64_publication_ordering_evidence_probe.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace Call-Boundary Publication Ordering

# Current Packet

## Prior Context

Step 1 traced the representative block-entry out-of-SSA edge-copy path.
Prepared/shared authority owns edge identity, source binding, destination
binding, move binding, and parallel-copy step metadata through
`make_prepared_edge_publication_lookups(...)`,
`prepare_edge_copy_source_facts(...)`, and
`prepare_block_entry_parallel_copy_edge_source_facts(...)`. AArch64 consumes
those facts in `lower_predecessor_select_parallel_copy_sources(...)` /
`lower_predecessor_join_source_publication(...)`; final AArch64 `lines`
serialization into the record remains target-local by design.

## Just Finished

Completed `plan.md` Step 2 evidence tracing for one representative
call-boundary publication path: normal call lowering around a retained BIR
`CallInst`, from prepared call facts through before-call argument publication,
the call record, and after-call result publication.

Representative path traced:
`dispatch.cpp::lower_block` obtains the prepared call plan with
`find_prepared_call_plan(...)`, emits scalar argument producers and stack
preservation publications, then calls `lower_before_call_moves(...)` before
`lower_call_instruction(...)` and `lower_after_call_moves(...)` after the call.
`lower_call_instruction(...)` requires the same call plan through
`require_prepared_call_plan(...)`, which delegates to
`prepare::find_indexed_prepared_call_plan(...)` over
`PreparedCallPlanLookups::calls_by_position`.

Prepared owners/helpers consumed by AArch64 calls:
`prepare::make_prepared_call_plan_lookups(...)` indexes
`PreparedCallPlansFunction::calls` by block/instruction and records immediate
argument, prior-preserved-value, first stack-preserved, and block-entry
republication lookup surfaces. `PreparedCallPlan` owns call identity,
`wrapper_kind`, direct or indirect callee facts, `memory_return`, ordered
`arguments`, optional `result`, `preserved_values`, and
`clobbered_registers`. The explicit call-boundary order is produced by
`prepare::plan_prepared_call_boundary_effects(...)`, which appends
`append_explicit_call_boundary_effects(...)` for the before-call bundle,
`append_preservation_call_boundary_effects(...)` for before-call preservation
home population, explicit after-call effects for the after-call bundle, and
after-call preservation republication. Each
`PreparedCallBoundaryEffectPlan` carries `effect_kind`, `phase`,
`block_index`, `instruction_index`, `order_index`, classification status,
destination kind/storage/ABI index, source endpoint, destination endpoint,
preservation route, and reason. `prepare::classify_prepared_call_boundary_move(...)`
binds each `PreparedMoveResolution` back to the matching
`PreparedCallArgumentPlan`, `PreparedCallResultPlan`, and `PreparedAbiBinding`.

AArch64 consumer/record path:
`lower_before_call_moves(...)` finds the `PreparedMovePhase::BeforeCall`
bundle with `find_move_bundle(...)`, computes `boundary_effects` with
`prepare::plan_prepared_call_boundary_effects(...)`, emits
`PreservationHomePopulation` effects first, then emits `ExplicitMove` effects
whose `effect.order_index` selects the concrete `bundle->moves[...]`, and
finally emits immediate ABI bindings from `bundle->abi_bindings`.
`lower_before_call_move(...)` reclassifies the selected move, uses
`find_indexed_prepared_value_home(...)` for the source home, consumes
`PreparedCallArgumentPlan::source_selection` when frame/local/byval/prior
preservation sources are needed, and builds a
`CallBoundaryMoveInstructionRecord` with the prepared bundle, move, source,
destination, phase, and authority fields. `make_call_boundary_move_instruction(...)`
then publishes a `MachineOpcode::CallBoundaryMove` record with operands,
defs, uses, and the prepared move payload. The call itself is published by
`lower_prepared_call_instruction(...)`, which copies prepared arguments,
result, preserved values, memory return, indirect callee, and
`clobbered_registers` into `CallInstructionRecord`; `make_call_instruction(...)`
emits the `InstructionFamily::Call` record and derives clobber effects through
`effects_from_prepared_call_clobbers(...)`. After the call,
`lower_after_call_moves(...)` iterates the prepared
`PreparedMovePhase::AfterCall` bundle in bundle order, lowers each
`CallResultAbi` move through `lower_after_call_move(...)`, and then emits
prepared preservation republication effects from
`prepare::plan_prepared_call_boundary_effects(...)`.

Classification:
- Publication order: prepared/shared for call-boundary effect identity and
  phase ordering. The shared planner assigns `PreparedCallBoundaryEffectPlan`
  entries and `order_index` values from prepared bundles and preserved values.
  AArch64 consumes those effects and uses `effect.order_index` to select the
  prepared move for before-call explicit moves; after-call result moves are
  consumed in prepared bundle order. AArch64 does add target-local scheduling
  around address materialization and source preservation in `dispatch.cpp`
  with `source_register_conflicts_with_materialized_address(...)`,
  `order_before_call_moves_for_source_preservation(...)`, and
  `materialize_call_boundary_source_to_destination(...)`. That ordering is a
  target-local hazard/spelling decision, not a new semantic call-boundary
  publication authority.
- Source binding: prepared/shared. Argument sources come from
  `PreparedCallArgumentPlan` fields, `PreparedCallArgumentSourceSelection`,
  `PreparedCallBoundaryEffectPlan::source`, the classified prepared move, and
  `PreparedValueHome` lookup. Result sources come from
  `PreparedCallResultPlan::source_*`, the prepared ABI binding, and the
  after-call move. Preservation sources come from
  `PreparedCallPreservedValue::preservation_source` or the value endpoint
  synthesized by `plan_prepared_call_boundary_effects(...)`.
- Destination binding: prepared/shared for ABI/value/home authority. Argument
  destinations are bound through `PreparedCallArgumentPlan`,
  `PreparedAbiBinding`, the selected `PreparedMoveResolution`, and
  `PreparedCallBoundaryEffectPlan::destination`. Result destinations are
  checked against `PreparedCallResultPlan`, `PreparedAbiBinding`, and
  `PreparedValueHome`. AArch64 converts prepared register names/placements into
  concrete register operands and views, but does not invent the ABI or value
  destination.
- Clobber publication order: prepared/shared for the clobber set as stored in
  `PreparedCallPlan::clobbered_registers`; `make_call_instruction(...)`
  serializes it through `effects_from_prepared_call_clobbers(...)` in vector
  order. The target-local action after the call is
  `clear_call_clobbered_emitted_scalar_registers(...)`, which invalidates
  AArch64 scalar-state aliases after the call record is emitted.
- Call-edge record order: mixed. The semantic call edge is prepared/shared via
  `PreparedCallPlan` and its before/after prepared move bundles. The concrete
  machine-record sequence in `dispatch.cpp` is target-local by design:
  scalar producer materialization, stack-preserved publications, address
  materializations, before-call boundary records, the call record, clobber
  scalar-state clearing, after-call boundary records, and result-source alias
  recording.
- Record emission order: target-local by design. `make_call_boundary_move_instruction(...)`
  orders operands as destination then source and emits defs/uses for the
  record; `make_call_instruction(...)` orders call operands as callee then
  arguments and publishes defs, uses, clobbers, preserves, and side effects.
  Those are record-shape and target spelling choices over prepared facts.
- Missing shared authority: no missing semantic authority proven for this
  representative call-boundary path. Compared with Step 1, call-boundary
  publication has the same split: prepared/shared facts own semantic
  publication identity, source binding, destination binding, and move/effect
  ordering, while AArch64 owns concrete record sequencing and hazard-driven
  local materialization order. The main difference is that Step 2 has an
  explicit shared call-boundary effect planner
  (`plan_prepared_call_boundary_effects(...)`) rather than the edge-copy
  source-facts query used in Step 1.

## Suggested Next

Trace one representative typed stack-source publication from prepared
stack-source facts to AArch64 records and compare it against the Step 1
edge-copy and Step 2 call-boundary models.

## Watchouts

- Do not change implementation files during the evidence-only trace.
- Do not add local ordering helpers before authority is proven.
- Keep publication expectation changes out of this packet.
- For Step 3, keep distinguishing prepared semantic authority from target-local
  record sequencing. Step 2 found target-local hazard ordering around
  call-boundary address materialization, but did not prove missing shared
  call-boundary publication authority.

## Proof

No build required by delegated proof; evidence-only `todo.md` update. No
`test_after.log` was produced because the packet explicitly required no build
or test proof.
