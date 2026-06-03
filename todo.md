Status: Active
Source Idea Path: ideas/open/96_aarch64_calls_deferred_move_publication_authority_audit.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Trace Scalar Producer Dispatch Bridge

# Current Packet

## Just Finished

Step 4 - Trace Scalar Producer Dispatch Bridge completed as an audit-only pass.
AST-backed queries and focused source windows traced the scalar producer bridge
from prepared source facts through dispatch-state mutation and AArch64
materialization spelling:

- Prepared source-producer authority:
  - `prepare::make_edge_publication_source_producers` records same-block
    `LoadLocal`, `LoadGlobal`, `Cast`, `Binary`, and `SelectMaterialization`
    producers by prepared value name. Duplicate producer publication clears
    the fact to `Unknown`, so consumers do not derive ambiguity locally.
  - `prepare::apply_source_producer_fact` copies the source producer kind,
    block label, instruction index, and instruction pointers into prepared edge
    publications. This is the durable shared authority for what produced a
    scalar value; AArch64 lowering only checks and consumes it.
  - `prepare::find_prepared_direct_global_select_chain_dependency`,
    `prepare::find_prepared_scalar_select_chain_materialization`,
    `prepare::evaluate_prepared_same_block_integer_constant`, and
    `prepare::find_prepared_same_block_load_local_stored_value_source` provide
    prepared-side facts for select chains, constants, and stored-value reload
    sources used by call-adjacent materialization.

- Call argument producer bridge:
  - `dispatch_prepared_block` calls `lower_scalar_call_argument_producers`
    before stack preservation, address materializations, before-call moves,
    indirect-callee materialization, missing frame-slot argument publication,
    and final call emission.
  - `lower_scalar_call_argument_producers` obtains
    `edge_publication_source_producers` from prepared lookups, with a local
    fallback rebuild only when prepared lookups are absent, and iterates call
    arguments using `find_prepared_call_argument_plan`.
  - For direct-global select chains,
    `materialize_direct_global_select_chain_call_argument` handles the
    target-local select-chain materialization before the generic scalar path.
  - `materialize_scalar_call_argument_value` checks local aggregate address
    publication permission from `PreparedCallArgumentPlan`, skips values
    already available in `BlockScalarLoweringState`, finds same-block prepared
    source producers, and recursively materializes scalar binary dependencies
    only for producer opcodes accepted by `is_scalar_call_argument_producer_opcode`.
  - When a binary producer is lowered, `lower_scalar_instruction` emits the
    AArch64 scalar instruction, then the result is retargeted to the prepared
    result register and published with `record_emitted_scalar_register`.

- Publication and materialization spelling:
  - `materialize_call_boundary_source_to_destination` is a bridge from a
    prepared call-boundary frame-slot source to a destination register when the
    source is not already in scalar state. It uses
    `prepared_call_boundary_source_value` to recover the prepared producer
    value, then calls `emit_value_publication_to_register` and records the
    emitted register in `BlockScalarLoweringState`.
  - `emit_value_publication_to_register` is target-local materialization
    spelling. It consumes prepared facts and helper lookups, but owns concrete
    `mov`, frame-slot `ldr`, pointer/global loads, sign/zero extension,
    truncation, select-chain emission, integer/floating compare materialization,
    bitwise/mul lowering, register view choice, and scratch-register routing.
  - Address and memory-derived materialization remains target-local spelling:
    `frame_slot_address`, `prepared_local_load_offset`,
    `emit_prepared_pointer_value_load_to_register`,
    `emit_prepared_global_load_to_register`, and prepared value-home emission
    use shared addressing/value-home facts but emit AArch64-specific text.

- Indirect-callee interaction:
  - `materialize_indirect_call_callee_to_prepared_register` uses
    `PreparedCallPlan::indirect_callee` as prepared call authority for the
    callee value name, storage encoding, bank, and target register name.
  - It then finds the same-block prepared source producer with
    `find_prepared_indirect_callee_source_producer`, optionally follows stored
    value and direct-global select-chain dependencies, and emits materialized
    lines through `emit_indirect_callee_value_to_register_with_csel`.
  - `emit_indirect_callee_value_to_register_with_csel` owns recursive
    AArch64-specific `cmp`/`csel` spelling, scratch selection via
    `indirect_callee_materialization_scratch_indices` and
    `choose_scratch_index`, and fallback publication through
    `emit_value_publication_to_register`. It records only the final prepared
    indirect callee register into scalar state.

Classification:

- `prepared-fact-consumption`: `lower_scalar_call_argument_producers`,
  `materialize_scalar_call_argument_value`,
  `materialize_call_boundary_source_to_destination`,
  `emit_value_publication_to_register`, and
  `materialize_indirect_call_callee_to_prepared_register` consume prepared
  source-producer, publication, value-home, addressing, call-argument, and
  indirect-callee facts.
- `target-local-calls-emission`: AArch64 materialization text, register view
  selection, scratch selection, select-chain `cmp`/`csel`, scalar ALU record
  retargeting, direct-global call-argument materialization, frame-slot/global
  load spelling, and indirect-callee materialization remain local codegen
  concerns.
- `dispatch-state-mutation`: `dispatch_prepared_block`,
  `record_emitted_scalar_register`,
  `find_emitted_scalar_register`, `emitted_scalar_value_available`,
  `current_block_entry_publication_register`, and
  `retarget_call_boundary_source_to_emitted_scalar` are the live scalar-state
  bridge. They publish or consult emitted registers after target-local
  materialization; they do not create new prepared facts.
- `intentionally-retained`: the bridge currently has a coherent split:
  prealloc/prepared lookups own source-producer and publication authority,
  calls/dispatch materialization owns target-local spelling, and
  `BlockScalarLoweringState` owns transient dispatch state around a call.

Follow-up boundary decision: no narrow implementation idea is recommended from
Step 4. A possible local cleanup around the shared use of
`emit_value_publication_to_register` by call-boundary and indirect-callee
materialization would prove mostly as helper organization, not as a compiler
capability or shared-authority repair. Retain this bridge unless Step 5 finds
the same scalar-state pattern leaking into late result publication authority.

## Suggested Next

Trace Step 5 next: result recording and late publication. Start from
`record_call_result_source_register`,
`retarget_fpr_call_result_store_value_to_emitted_scalar`,
`materialize_missing_frame_slot_call_arguments`, and
`publish_stack_preserved_call_values`, then separate prepared result/publication
facts from late dispatch-state mutation and AArch64 emission spelling.

## Watchouts

- This is an audit-only plan until a later step creates narrow follow-up ideas.
- Do not edit implementation or test expectation files during audit packets.
- Do not treat AArch64 scratch choice, call spelling, materialization spelling,
  or machine-record construction as shared authority without traced evidence.
- Do not reopen ideas 93-95 as local-owner routes.
- `dispatch_prepared_block` remains the dispatch-state mutation hub. It orders
  scalar producer emission before call-boundary moves and records only emitted
  scalar facts, not prepared source facts.
- `emit_value_publication_to_register` deliberately serves both call-boundary
  and indirect-callee materialization. Treat its output spelling as target-local
  unless Step 5 proves a prepared-publication authority gap.
- Indirect-callee materialization consumes prepared call-plan/source-producer
  facts, but target register parsing, scratch choice, recursive `csel`
  materialization, and final scalar-state recording are local calls/codegen
  responsibilities.
- Steps 2 through 4 did not identify a narrow implementation boundary. Avoid
  turning helper extraction or materialization reshuffling into capability
  progress unless Step 5 ties late publication to a concrete shared-authority
  gap.

## Proof

Audit-only packet. Used `c4c-clang-tools` AST queries for definitions,
callees, callers, and `BlockScalarLoweringState` references around
`lower_scalar_call_argument_producers`,
`materialize_scalar_call_argument_value`,
`find_prepared_scalar_call_argument_source_producer`,
`materialize_call_boundary_source_to_destination`,
`emit_value_publication_to_register`,
`materialize_indirect_call_callee_to_prepared_register`,
`emit_indirect_callee_value_to_register_with_csel`,
`prepare::make_edge_publication_source_producers`,
`prepare::find_prepared_direct_global_select_chain_dependency`,
`prepare::find_prepared_same_block_load_local_stored_value_source`, and
`dispatch_prepared_block`. No build or backend test proof required, and no
`test_after.log` was generated because no implementation or expectation files
were touched.
