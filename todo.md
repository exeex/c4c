Status: Active
Source Idea Path: ideas/open/96_aarch64_calls_deferred_move_publication_authority_audit.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Synthesize Audit Result And Follow-Up Ideas

# Current Packet

## Just Finished

Step 6 - Synthesize Audit Result And Follow-Up Ideas completed as an
audit-only pass. The Step 1-5 traces classify every deferred cluster from idea
92 and the active source idea without finding a narrow proofable authority
boundary for implementation follow-up.

Step-by-step synthesis:

- Step 1 mapped the deferred clusters to concrete producers, consumers, and
  emission sites. The map showed four audit families: before-call move bundle
  lowering; after-call, return, value, and preservation lowering; scalar
  producer dispatch bridge; and result recording plus late publication.
- Step 2 traced before-call move bundle lowering from
  `prepare::plan_prepared_call_boundary_effects`,
  `append_explicit_call_boundary_effects`, prepared before-call move bundles,
  argument endpoints, preservation effects, and immediate argument facts into
  `lower_before_call_moves`, `lower_before_call_move`,
  `lower_before_call_immediate_binding`, and dispatch scalar-state retargeting
  and recording.
- Step 3 traced after-call/result moves, before-return/value moves,
  preservation home population, and preservation republication through
  prepared move bundles, result endpoints, value homes, preserved-value routes,
  `lower_after_call_moves`, `lower_before_return_moves`, `lower_value_moves`,
  `record_call_result_source_register`, and dispatch return/result
  deduplication.
- Step 4 traced scalar producer publication through prepared source-producer
  facts, call-argument plans, same-block producer lookups, publication helper
  facts, `lower_scalar_call_argument_producers`,
  `materialize_scalar_call_argument_value`,
  `emit_value_publication_to_register`, and indirect-callee materialization.
- Step 5 traced result recording and late publication through prepared call
  result plans, after-call result lane bindings, frame-slot call argument
  selections, stack-preserved value lookups,
  `record_call_result_source_register`,
  `retarget_fpr_call_result_store_value_to_emitted_scalar`,
  `materialize_missing_frame_slot_call_arguments`, and
  `publish_stack_preserved_call_values`.

Final classification:

- `before-call move bundle lowering`: intentionally retained. Prealloc owns the
  prepared move bundle, call-boundary effect, argument endpoint, preservation,
  ABI binding, immediate argument, value-home, byval, F128, and frame-slot
  facts. Calls/codegen owns AArch64 register views, scratch choice, stack-store
  spelling, operand construction, call-boundary records, and select-chain
  materialization. Dispatch owns ordering around address materialization,
  source retargeting, destination/source alias recording, and live scalar-state
  mutation.
- `after-call, return, value, and preservation lowering`: intentionally
  retained. Prepared facts own result and preservation routes, move bundles,
  value homes, ABI endpoints, and republication effects. Calls/codegen owns
  target-local result/register/store/F128/callee-saved instruction spelling.
  Dispatch owns result publication order, call-clobbered scalar clearing,
  return-publication deduplication, and memory-record retargeting.
- `scalar producer dispatch bridge`: intentionally retained. Prepared lookups
  own source-producer, publication, select-chain, constant, stored-value,
  call-argument, and indirect-callee facts. Calls/dispatch materialization owns
  AArch64 `mov`, `ldr`, extension/truncation, ALU, compare/select, scratch
  routing, direct-global, local aggregate address, and indirect-callee spelling.
  `BlockScalarLoweringState` owns transient emitted-register availability.
- `result recording and late publication`: intentionally retained. Prepared
  facts own result registers, lane bindings, frame-slot argument moves,
  source selections, stack-preserved values, and preservation lookup indexes.
  Dispatch owns when call results/lane registers are recorded, when missing
  frame-slot arguments are materialized, and when FPR stores are retargeted.
  Calls/codegen owns target-local machine-record and instruction spelling.

Generated follow-up ideas: none. The audit did not find a cluster classified
`needs-narrow-implementation-idea`. The repeated pattern is a coherent
three-way boundary, not a shared-authority leak: prealloc/prepared plans derive
durable facts, dispatch mutates transient scalar state and ordering, and
calls/codegen emits AArch64-specific records and instructions. The visible
cleanup seams would prove mainly by helper shape, file size, or line count,
which the source idea rejects as insufficient authority evidence.

Close-readiness notes for plan-owner review:

- The source idea's deferred clusters are all classified.
- No implementation or test expectation files were touched.
- No follow-up source ideas were generated because no narrow, proofable
  authority boundary was found.
- Plan-owner can review this active plan for closure as an audit-only route
  whose durable conclusion is intentional retention of the traced clusters.

## Suggested Next

Supervisor should send the active plan to plan-owner for close review. No
executor implementation packet is recommended from this audit.

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
- The intentionally retained deferred clusters are:
  before-call move bundle lowering; after-call/result/return/value and
  preservation lowering; scalar producer dispatch bridge; result recording and
  late publication.
- Do not convert these retained clusters into helper-extraction, file-shrink,
  or expectation-rewrite work and claim capability progress.
- Any future follow-up should start from new evidence of a shared prepared
  authority gap or dispatch-state leak, not from the current audit alone.

## Proof

Audit-only packet. Read `plan.md`, the active source idea, current `todo.md`,
and recent `todo.md` history for Steps 1-5. No clang-tools re-check was needed
for synthesis, no build or backend test proof was required, and no
`test_after.log` was generated because no implementation or expectation files
were touched.
