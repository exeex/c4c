Status: Active
Source Idea Path: ideas/open/96_aarch64_calls_deferred_move_publication_authority_audit.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Trace Result Recording And Late Publication

# Current Packet

## Just Finished

Step 5 - Trace Result Recording And Late Publication completed as an
audit-only pass. AST-backed queries and focused source windows traced the late
result/publication paths from prepared call facts through dispatch-state
mutation and target-local AArch64 emission:

- Call result source register recording:
  - `prepare::build_call_result_plan` creates `PreparedCallResultPlan` from the
    after-call move bundle, including destination value id, source ABI register
    name/placement, source bank, contiguous width, occupied registers, and the
    prepared destination home. This is the prepared authority for the primary
    call result register.
  - `prepare::publish_prepared_after_call_result_lane_bindings` derives
    `PreparedAfterCallResultLaneBinding` facts from `AfterCall` bundles whose
    ABI binding destination is `CallResultAbi` register storage, then indexes
    them by block, instruction, and prepared value id.
  - `record_call_result_source_register` consumes `PreparedCallPlan::result`
    and `find_prepared_after_call_result_lane_binding`; it parses/converts the
    prepared ABI register and records the destination value in
    `BlockScalarLoweringState` with `record_emitted_scalar_register`.
  - `dispatch_prepared_block` records the primary result immediately after
    `lower_call_instruction` and `clear_call_clobbered_emitted_scalar_registers`,
    lowers after-call moves, then calls `record_call_result_source_register`
    again with `result_lanes_only=true` so lane bindings are recorded after
    destination moves.

- FPR result store retargeting:
  - `retarget_fpr_call_result_store_value_to_emitted_scalar` runs from the
    ordinary memory path in `dispatch_prepared_block` after `lower_memory_instruction`.
    It only handles `StoreLocal` of named `F32`/`F64` values whose emitted
    memory record is a matching store.
  - The helper does not create prepared facts. It consults
    `BlockScalarLoweringState` for a prior `CallAbi` FPR emission recorded by
    `record_call_result_source_register`, then rewrites the memory record value
    operand to the emitted register with `make_register_operand`.
  - The mutation is late dispatch-state repair for the machine record; the
    AArch64 store record and register operand spelling remain target-local.

- Missing frame-slot call arguments:
  - `materialize_missing_frame_slot_call_arguments` runs after normal
    before-call moves and call-boundary source materialization but before final
    call emission. It only considers prepared call arguments whose source is a
    frame slot, destination is a single GPR, and whose source value is not
    already present in `BlockScalarLoweringState`.
  - It consumes the prepared source home, the prepared frame-slot call argument
    move from `find_prepared_frame_slot_call_argument_move`, and
    `PreparedCallArgumentSourceSelection` to choose frame-slot address/value or
    prior-preservation source memory.
  - It emits a target-local dispatch bridge `CallBoundaryMoveInstructionRecord`
    through `make_call_boundary_move_instruction` and records the destination
    register in scalar state. This is late target-local materialization of an
    already-prepared argument plan, not new call-argument authority.

- Stack-preserved value publication:
  - `prepare::build_call_preserved_values` creates
    `PreparedCallPreservedValue` records from liveness, value homes, frame
    slots, stack assignments, and callee-saved register assignments. For stack
    routes it records value id/name, slot id, stack offset/size/alignment, and
    preservation source/destination endpoints.
  - `prepare::make_prepared_call_plan_lookups` indexes the first stack-preserved
    values per call, suppressing later calls already reached by a prior stack
    preservation through `prior_stack_preserved_reaches_call`.
  - `publish_stack_preserved_call_values` consumes
    `first_indexed_stack_preserved_values_for_call`; for each stack-preserved
    value it prefers an emitted GPR from `BlockScalarLoweringState`, otherwise
    falls back to a prepared register home, chooses a W/X view from stack size,
    and emits an AArch64 `str` to the prepared frame-slot address.
  - This path is prepared preservation/publication fact consumption plus
    target-local store spelling. It does not mutate scalar state and does not
    derive preservation authority locally.

Classification:

- `prepared-fact-consumption`: `record_call_result_source_register` consumes
  `PreparedCallPlan::result` and prepared after-call lane bindings;
  `materialize_missing_frame_slot_call_arguments` consumes prepared
  frame-slot argument plans, source homes, and source selections;
  `publish_stack_preserved_call_values` consumes prepared stack-preservation
  lookups and value homes.
- `dispatch-state-mutation`: `record_call_result_source_register` publishes
  call result/lane registers into `BlockScalarLoweringState`;
  `retarget_fpr_call_result_store_value_to_emitted_scalar` and
  `materialize_missing_frame_slot_call_arguments` consult the same transient
  scalar state to avoid stale or missing machine operands.
- `target-local-calls-emission`: register parsing/conversion, memory record
  operand retargeting, dispatch bridge record construction, frame-slot address
  spelling, and the emitted `str` for preserved stack values remain AArch64
  local codegen.
- `intentionally-retained`: no Step 5 helper re-derives call result,
  after-call lane, frame-slot argument, or stack-preservation authority. The
  current split is coherent: prealloc owns prepared facts and lookups,
  dispatch owns sequencing and live scalar-state mutation, and calls/codegen
  owns target-local machine record or instruction spelling.

Follow-up boundary decision: no narrow implementation idea is recommended from
Step 5. The only visible cleanup boundary would be local organization around
late scalar-state consumers (`record_call_result_source_register`,
`retarget_fpr_call_result_store_value_to_emitted_scalar`, and
`materialize_missing_frame_slot_call_arguments`), but the trace does not show a
shared-authority gap or proofable compiler capability boundary. Retain these
paths unless Step 6 synthesis finds a cross-step pattern worth documenting as a
separate source idea.

## Suggested Next

Run Step 6 synthesis: summarize Steps 1-5 and decide whether the audit should
close with intentionally retained boundaries or create only a narrow follow-up
idea backed by repeated traced evidence.

## Watchouts

- This is an audit-only plan until a later step creates narrow follow-up ideas.
- Do not edit implementation or test expectation files during audit packets.
- Do not treat AArch64 scratch choice, call spelling, materialization spelling,
  or machine-record construction as shared authority without traced evidence.
- Do not reopen ideas 93-95 as local-owner routes.
- `dispatch_prepared_block` remains the dispatch-state mutation hub. It orders
  scalar producer emission, stack-preserved publication, before-call
  materialization, call emission, result recording, after-call moves, and late
  memory retargeting.
- Step 5 did not find a prepared-publication authority gap. Avoid turning
  local helper extraction or scalar-state consumer grouping into capability
  progress.
- Stack-preserved publication emits target-local stores from prepared
  preservation facts and current scalar/register homes; it intentionally does
  not record new scalar-state facts.
- FPR result store retargeting depends on call result source registers already
  published into scalar state. If future work changes result recording order,
  this late memory path is a direct watchpoint.

## Proof

Audit-only packet. Used `c4c-clang-tools` AST queries for definitions,
callees, callers, and dispatch ordering around
`record_call_result_source_register`,
`retarget_fpr_call_result_store_value_to_emitted_scalar`,
`materialize_missing_frame_slot_call_arguments`,
`publish_stack_preserved_call_values`, and `dispatch_prepared_block`; focused
source windows confirmed prepared result plans, after-call lane bindings,
stack-preserved lookup construction, dispatch-state mutation, and target-local
emission spelling. No build or backend test proof required, and no
`test_after.log` was generated because no implementation or expectation files
were touched.
