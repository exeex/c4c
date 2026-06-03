Status: Active
Source Idea Path: ideas/open/96_aarch64_calls_deferred_move_publication_authority_audit.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Trace Before-Call Move Bundle Lowering

# Current Packet

## Just Finished

Step 2 - Trace Before-Call Move Bundle Lowering completed as an audit-only
pass. AST-backed queries and focused source windows traced this path:

- Prepared fact production:
  - `prepare::plan_prepared_call_boundary_effects` calls
    `append_explicit_call_boundary_effects` and
    `append_preservation_call_boundary_effects`.
  - `append_explicit_call_boundary_effects` consumes each
    `PreparedMoveBundle::moves` entry, calls
    `classify_prepared_call_boundary_move`, and records a
    `PreparedCallBoundaryEffectPlan` with phase, order index,
    classification status, destination kind/storage, ABI index, source
    endpoint, destination endpoint, and reason.
  - Argument/result endpoint authority is selected in prealloc through
    `make_argument_source_endpoint`, `make_argument_destination_endpoint`,
    `make_result_source_endpoint`, and `make_result_destination_endpoint`;
    preservation endpoints are added separately from
    `PreparedCallPlan::preserved_values`.
  - `PreparedMovePhase::BeforeCall` bundle lookup is indexed by
    `find_indexed_prepared_move_bundle`; the prepared lookup index also records
    before-call argument moves by block/instruction/ABI position for later
    direct lookup.

- AArch64 prepared-fact consumption and record construction:
  - `dispatch_prepared_block` obtains the active `PreparedCallPlan`, then calls
    `lower_scalar_call_argument_producers`, `publish_stack_preserved_call_values`,
    `lower_address_materializations`, and finally `lower_before_call_moves`
    before call emission.
  - `lower_before_call_moves` calls local `find_move_bundle` for
    `PreparedMovePhase::BeforeCall`; if no bundle is found it creates an empty
    synthetic BeforeCall bundle so preservation home population and stack-base
    setup still have a call-boundary context.
  - `lower_before_call_moves` adds outgoing stack-base setup when the prepared
    call plan needs outgoing stack argument bytes, calls
    `prepare::plan_prepared_call_boundary_effects(call_plan, bundle, nullptr)`,
    lowers `PreservationHomePopulation` effects with
    `make_callee_saved_preservation_home_population`, lowers explicit move
    effects through `lower_before_call_move`, and lowers immediate ABI bindings
    through `lower_before_call_immediate_binding`.
  - `lower_before_call_move` consumes the prepared move/effect/classification
    and AArch64-local prepared adjuncts such as value homes, F128 carriers,
    byval lanes, prior-preservation selections, frame-slot/local-frame address
    selections, and register views; it emits a
    `CallBoundaryMoveInstructionRecord` through
    `make_call_boundary_move_instruction` /
    `make_call_boundary_machine_instruction`.
  - `lower_before_call_immediate_binding` consumes prepared ABI bindings and
    prepared immediate call-argument lookups, then chooses AArch64 stack store
    or register move spelling and emits a synthetic
    `CallBoundaryMoveInstructionRecord` for the immediate source.

- Dispatch-state mutation and target-local emission spelling:
  - `dispatch_prepared_block` first retargets returned before-call records with
    `retarget_call_boundary_source_to_emitted_scalar`; this rewrites the record
    source from prepared memory/register to an already emitted scalar register
    and may adjust GP destination view. It does not create prepared facts.
  - Address-conflicting moves are emitted before address materializations and
    immediately passed to `record_call_boundary_destination`, which records the
    destination register in `BlockScalarLoweringState`.
  - Non-conflicting moves are deferred, materialized addresses are emitted and
    recorded with `record_emitted_scalar_register`, then deferred moves are
    ordered by `order_before_call_moves_for_source_preservation`.
  - For each deferred move, `call_boundary_move_reloads_materialized_address`
    can suppress duplicate reloads; otherwise dispatch retargets again, tries
    `materialize_call_boundary_source_to_destination`, and either emits the
    materialization or records/emits the original call-boundary move.
  - `materialize_call_boundary_source_to_destination` is target-local spelling:
    it queries `prepared_call_boundary_source_value`, chooses reserved AArch64
    scratch GP registers, calls `emit_value_publication_to_register`, records
    the emitted scalar in `BlockScalarLoweringState`, and returns
    `make_select_chain_materialization_instruction`.
  - `record_call_boundary_destination` and
    `record_call_boundary_source_in_destination` are dispatch-state mutation
    helpers; they update emitted scalar state from the destination register or
    source alias after a call-boundary move.

Classification:

- `prepared-fact-consumption`: `lower_before_call_moves`,
  `lower_before_call_move`, and `lower_before_call_immediate_binding` consume
  prepared move bundles, call plans, ABI bindings, classifications, and lookup
  facts without owning their derivation.
- `target-local-calls-emission`: AArch64 register views, scratch choice,
  stack-store spelling, byval/F128/frame-slot operand construction,
  call-boundary machine records, and select-chain materialization remain local
  emission concerns.
- `dispatch-state-mutation`: `dispatch_prepared_block`,
  `retarget_call_boundary_source_to_emitted_scalar`,
  `materialize_call_boundary_source_to_destination`,
  `record_call_boundary_destination`, and
  `record_call_boundary_source_in_destination` mutate emitted-scalar state or
  rewrite records at the call boundary.
- `intentionally-retained`: the before-call cluster crosses prepared fact
  consumption, AArch64 call-boundary record spelling, address-materialization
  ordering, and scalar-state publication. The trace does not expose a narrow
  proofable implementation boundary that would improve authority without
  either moving target-local AArch64 spelling into shared prealloc or creating a
  broad dispatch/calls split.

Follow-up boundary decision: no new implementation idea is recommended from
Step 2. The narrowest apparent seams, such as extracting before-call move
ordering/retargeting or immediate binding record spelling, are local structural
cleanup seams rather than authority gaps; they would be hard to prove without
line-count or helper-shape metrics and risk weakening the audit's overfit
guard. Retain this cluster unless a later trace finds a repeated dispatch-state
mutation boundary shared with after-call/result publication.

## Suggested Next

Trace Step 3 next: after-call result moves, return/value moves, preservation,
and republication. Start from `prepare::plan_prepared_call_boundary_effects`
with the AfterCall bundle and `PreparedCallPlan::preserved_values`, then trace
`lower_after_call_moves`, `record_call_result_source_register`,
`retarget_fpr_call_result_store_value_to_emitted_scalar`, `lower_value_moves`,
and `lower_before_return_moves` through `dispatch_prepared_block`.

## Watchouts

- This is an audit-only plan until a later step creates narrow follow-up ideas.
- Do not edit implementation or test expectation files during audit packets.
- Do not treat AArch64 scratch choice, call spelling, materialization spelling,
  or machine-record construction as shared authority without traced evidence.
- Do not reopen ideas 93-95 as local-owner routes.
- `dispatch_prepared_block` is the dispatch-state mutation hub; it records
  call-boundary destinations/sources and call results around call emission, so
  later traces should distinguish mutation of `BlockScalarLoweringState` from
  prepared fact consumption.
- `emit_value_publication_to_register` appears in both dispatch publication and
  calls paths; classify its AArch64 materialization spelling separately from
  the prepared publication/source-producer facts that feed it.
- Indirect-callee materialization uses prepared source producers but also owns
  target-local scratch choice and concrete `csel`/materialization emission.
- Step 2 did not identify a narrow implementation boundary. Do not turn
  before-call helper extraction, immediate binding spelling, or move-ordering
  reshuffles into capability progress unless a later step ties the same
  dispatch-state mutation pattern to a broader prepared/publication authority
  gap.

## Proof

Audit-only packet. Used `c4c-clang-tools` AST queries for callees/callers around
`lower_before_call_moves`, `dispatch_prepared_block`,
`prepare::plan_prepared_call_boundary_effects`,
`append_explicit_call_boundary_effects`,
`lower_before_call_move`, `lower_before_call_immediate_binding`,
`materialize_call_boundary_source_to_destination`,
`record_call_boundary_destination`, and
`retarget_call_boundary_source_to_emitted_scalar`. No build or backend test
proof required, and no `test_after.log` was generated because no implementation
or expectation files were touched.
