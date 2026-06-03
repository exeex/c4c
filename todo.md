Status: Active
Source Idea Path: ideas/open/96_aarch64_calls_deferred_move_publication_authority_audit.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Trace After-Call, Return, Value, And Preservation Lowering

# Current Packet

## Just Finished

Step 3 - Trace After-Call, Return, Value, And Preservation Lowering completed
as an audit-only pass. AST-backed queries and focused source windows traced
these paths:

- Shared prepared fact production:
  - `prepare::plan_prepared_call_boundary_effects` calls
    `append_explicit_call_boundary_effects` for before/after bundles and
    `append_preservation_call_boundary_effects` for both preservation-home
    population and preservation republication.
  - `append_explicit_call_boundary_effects` classifies each prepared move with
    `classify_prepared_call_boundary_move`; result moves get their source and
    destination endpoints from `make_result_source_endpoint` and
    `make_result_destination_endpoint`.
  - `append_preservation_call_boundary_effects` consumes
    `PreparedCallPlan::preserved_values`, skips unknown routes, and creates
    either a preservation-source to storage endpoint or storage to
    republication endpoint effect.
  - `PreparedCallPlan::preserved_values` is built in prealloc from liveness,
    value homes, frame slots, stack assignments, and callee-saved register
    assignments. This is shared prepared authority; AArch64 lowering only
    consumes the resulting route, storage, register, slot, size, and placement
    facts.

- After-call result move lowering:
  - `dispatch_prepared_block` emits the call through `lower_call_instruction`,
    clears call-clobbered scalar state, records result source registers, calls
    `lower_after_call_moves`, records each returned call-boundary destination,
    then records result lanes again with `result_lanes_only=true`.
  - `lower_after_call_moves` looks up a `PreparedMovePhase::AfterCall` bundle
    with local `find_move_bundle`; explicit moves are lowered with
    `lower_after_call_move`.
  - `lower_after_call_move` consumes the prepared call plan, after-call bundle,
    `classify_prepared_call_boundary_move`, ABI binding, destination value
    home, and F128 carrier facts. It validates selected GPR, scalar FPR, F128,
    and stack-result publication shapes against prepared result authority, then
    emits `CallBoundaryMoveInstructionRecord` or a target-local stack store.
  - If no after-call bundle exists, `lower_after_call_moves` still builds a
    synthetic bundle for republication effects and calls
    `prepare::plan_prepared_call_boundary_effects(call_plan, nullptr,
    &republication_bundle)`.

- Result recording and store retargeting:
  - `record_call_result_source_register` uses `PreparedCallPlan::result`,
    prepared value homes, prepared register placement/bank, and after-call
    result-lane bindings to publish call ABI result registers into
    `BlockScalarLoweringState`.
  - The first call records the scalar result before after-call moves so later
    instructions can see the ABI source; the second records result lanes after
    after-call moves. Both are dispatch-state mutation, not prepared fact
    derivation.
  - `retarget_fpr_call_result_store_value_to_emitted_scalar` is invoked from
    ordinary memory lowering in `dispatch_prepared_block`; it detects F32/F64
    store-local values, consults emitted scalar state for a call-ABI FPR, and
    rewrites the memory store value operand to that emitted register.

- Return and value move lowering:
  - `lower_before_return_moves` consumes a `PreparedMovePhase::BeforeReturn`
    bundle and prepared value homes. It lowers only moves to
    `FunctionReturnAbi` register destinations, building the destination from
    prepared register authority and the source from either prepared register
    home or prepared frame-slot memory.
  - `dispatch_prepared_block` suppresses duplicate return publication via
    `before_return_publication_already_emitted`, records accepted return
    publication with `record_before_return_publication`, then lowers the
    prepared return terminator with the updated scalar state.
  - `lower_value_moves` consumes only `BlockEntry` and `BeforeInstruction`
    prepared move bundles. It uses prepared value homes for register and stack
    destinations, prior stack-preserved values for before-instruction sources,
    and block-entry republication effects from prepared lookups; dispatch then
    records destinations and source aliases when those emitted records affect
    scalar state.

- Preservation and republication:
  - Before calls, `publish_stack_preserved_call_values` uses prepared call-plan
    lookups and `first_indexed_stack_preserved_values_for_call` to publish the
    first stack-preserved values for a call from either an already emitted GP
    scalar or the prepared register home into the prepared frame slot. The
    `str` spelling and `make_select_chain_materialization_instruction` record
    are target-local AArch64 emission; the preservation route and slot facts
    are prepared authority.
  - After calls, preservation republication effects from
    `plan_prepared_call_boundary_effects` are consumed by
    `make_callee_saved_preservation_home_republication_instruction`, which
    only emits callee-saved-register route republications when the prepared
    storage and value endpoints provide register names, banks, value identity,
    size/view, and placements.

Classification:

- `prepared-fact-consumption`: `lower_after_call_moves`,
  `lower_after_call_move`, `lower_before_return_moves`, `lower_value_moves`,
  `publish_stack_preserved_call_values`, and
  `make_callee_saved_preservation_home_republication_instruction` consume
  prepared move bundles, result plans, ABI bindings, value homes, preservation
  routes, and prepared lookup indexes.
- `target-local-calls-emission`: AArch64 register operand construction, scalar
  register views, FPR/GPR/F128/store selection, frame-slot store spelling,
  select-chain materialization records, and call-boundary machine instruction
  records remain local calls/codegen concerns.
- `dispatch-state-mutation`: `dispatch_prepared_block`,
  `record_call_result_source_register`,
  `retarget_fpr_call_result_store_value_to_emitted_scalar`,
  `before_return_publication_already_emitted`,
  `record_before_return_publication`, `record_call_boundary_destination`, and
  `record_call_boundary_source_in_destination` mutate, consult, or suppress
  `BlockScalarLoweringState` publications around calls, memory stores, and
  returns.
- `intentionally-retained`: the traced Step 3 paths deliberately cross shared
  prepared authority, target-local AArch64 call/result spelling, and dispatch
  scalar-state mutation. The current boundary is coherent: prealloc derives the
  durable call/result/preservation facts; calls.cpp emits target records from
  those facts; dispatch.cpp owns when emitted scalar state is recorded,
  retargeted, cleared, or deduplicated.

Follow-up boundary decision: no narrow implementation idea is recommended from
Step 3. The only plausible seams are local structural cleanup seams, such as a
small result-recording helper split or grouping after-call republication
spelling, but those would prove mainly by helper shape rather than by a
compiler capability boundary. Retain these paths unless Step 4 or Step 5 finds
the same dispatch-state mutation pattern leaking prepared-publication
authority across a broader scalar producer or late-publication boundary.

## Suggested Next

Trace Step 4 next: scalar producer dispatch bridge. Start from prepared call
argument source producers and publication facts, then trace
`lower_scalar_call_argument_producers`, `emit_value_publication_to_register`,
address/materialization helpers, indirect-callee materialization, and the
dispatch `BlockScalarLoweringState` bridge.

## Watchouts

- This is an audit-only plan until a later step creates narrow follow-up ideas.
- Do not edit implementation or test expectation files during audit packets.
- Do not treat AArch64 scratch choice, call spelling, materialization spelling,
  or machine-record construction as shared authority without traced evidence.
- Do not reopen ideas 93-95 as local-owner routes.
- `dispatch_prepared_block` remains the dispatch-state mutation hub; it records
  call-boundary destinations/sources, call results, before-return
  publications, and FPR call-result store retargeting around call and memory
  emission.
- `emit_value_publication_to_register` appears in both dispatch publication and
  calls paths; classify its AArch64 materialization spelling separately from
  the prepared publication/source-producer facts that feed it in Step 4.
- Indirect-callee materialization uses prepared source producers but also owns
  target-local scratch choice and concrete `csel`/materialization emission.
- Steps 2 and 3 did not identify a narrow implementation boundary. Do not turn
  before/after-call helper extraction, result-recording splits, return-move
  spelling, or preservation republication reshuffles into capability progress
  unless a later step ties the same dispatch-state mutation pattern to a
  broader prepared/publication authority gap.

## Proof

Audit-only packet. Used `c4c-clang-tools` AST queries for definitions,
callees, and callers around `prepare::plan_prepared_call_boundary_effects`,
`build_call_preserved_values`, `PreparedMovePhase::AfterCall`,
`PreparedCallPlan::preserved_values`, `lower_after_call_moves`,
`lower_before_return_moves`, `lower_value_moves`,
`record_call_result_source_register`,
`retarget_fpr_call_result_store_value_to_emitted_scalar`,
`publish_stack_preserved_call_values`,
`make_callee_saved_preservation_home_republication_instruction`, and
`dispatch_prepared_block`. No build or backend test proof required, and no
`test_after.log` was generated because no implementation or expectation files
were touched.
