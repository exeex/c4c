Status: Active
Source Idea Path: ideas/open/118_aarch64_calls_deferred_cluster_post_contract_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Closure Readiness Package

# Current Packet

## Just Finished

Step 4 of `plan.md` created bounded follow-up `ideas/open/` files for each
actionable Step 3 disposition and recorded explicit no-new-idea rationale for
clusters already closed, intentionally local, or not actionable as standalone
work.

## Step 1 Closed-Route Evidence

- Idea 92 deferred these calls-side clusters for later shared-authority
  evidence: before-call move bundle lowering; after-call, return, value, and
  preservation lowering; scalar producer dispatch bridge; result recording and
  late publication. It also kept ordinary call-boundary move and ABI-binding
  record construction local unless directly coupled to one of those deferred
  clusters.
- Idea 93 closed the stack/frame-slot operand owner. Duplicate-work guardrail:
  do not reopen stack call argument destinations, selected frame-slot sources,
  sret/local-frame address sources, aggregate stack sources, endpoint stack
  storage, prior stack-preserved sources, or memory-return frame-slot storage;
  future calls work must consume that local owner instead of reselecting homes
  or rebuilding prepared call, move-bundle, result, preservation, or aggregate
  transport facts under a new name.
- Idea 94 closed the f128 carrier operand owner. Duplicate-work guardrail: do
  not reopen prepared f128 carrier completion, module fallback lookup, or
  q-register operand rendering; future calls work must not invent new shared
  f128 transport authority or absorb ordinary call-boundary record ownership
  through an f128 helper.
- Idea 95 closed the immediate scalar argument publication owner.
  Duplicate-work guardrail: do not reopen same-block immediate cast lookup,
  immediate bit extraction, GP/FP destination views, FP immediate scratch
  materialization, or inline-asm instruction construction; keep
  `lower_before_call_immediate_binding` as the prepared immediate consumer.
- Idea 114 adds a prepared outgoing stack argument area fact owned by shared
  prealloc/call planning. Calls-side follow-ups may consume the call-level area
  as reservation/address authority, while AArch64 still owns `x16`, stack
  adjustment/restoration, store ordering, and target ABI placement. Guardrail:
  do not duplicate outgoing stack area work or rederive the total area from
  per-argument destinations in a new calls owner.
- Idea 116 adds shared prepared dispatch facts for edge-copy materializable
  producer classification, edge-publication source consistency, current-block
  join instruction routing, and direct-global select-chain dependency lookup.
  Guardrail: do not reopen dispatch prepared-producer, current-block
  publication, join-routing, or select-chain contract work; calls-side scalar
  producer bridge analysis should look for consumers of those shared facts, not
  new AArch64 producer discovery.
- Idea 117 adds shared prepared comparison facts for fused-compare operand
  producers, materialized compare join/publication facts, and current-block
  publication registers. Guardrail: do not reopen comparison publication or
  fused-compare routing; calls-side result/publication analysis may consume the
  current-block publication contract but must keep AArch64 compare/branch
  spelling out of shared code.

## Step 2 Function-Level Cluster Map

- Before-call move bundle lowering:
  `lower_before_call_move`, plus coupled helpers
  `make_register_operand_from_prepared_authority`,
  `make_selected_call_argument_source`,
  `make_prior_preserved_call_argument_source`,
  `source_memory_after_outgoing_stack_reservation`,
  `materialize_fp_stack_call_argument_source`,
  `materialize_f128_stack_call_argument_source`,
  `lower_before_call_immediate_binding`,
  `find_immediate_argument_in_call_plan`, and closed local owners
  `StackFrameSlotCallOperandOwner`, `F128CarrierCallOperandOwner`, and
  `ImmediateScalarCallArgumentPublicationOwner`. Consumed facts are
  `PreparedCallPlan`, `PreparedMoveBundle`, `PreparedCallBoundaryEffectPlan`,
  `PreparedMoveResolution`, prepared value homes, prepared ABI bindings,
  prepared source selections, prepared f128 carriers, indexed immediate
  argument lookups, and the idea-114
  `PreparedCallPlan::outgoing_stack_argument_area`. State mutation is mostly
  construction of `CallBoundaryMoveInstructionRecord` and immediate/memory
  machine records; only address/byval paths also mark
  `source_memory_materializes_address` or retag the move reason. Retained
  AArch64-local emission is register-view conversion, ABI register spelling,
  q-register/f128 carrier rendering, `x16` outgoing stack base addressing,
  inline-asm FP/f128 materialization, stack store records, and diagnostics for
  incomplete prepared authority.
- After-call result/value lowering:
  `lower_after_call_move`, with `make_register_operand_from_prepared_authority`,
  `register_name_with_expected_view`, `scalar_fp_view_from_register_name`,
  `scalar_integer_register_view_from_size`, `scalar_integer_type_from_size`,
  `make_call_boundary_move_instruction`, and `make_memory_instruction`.
  Consumed facts are the prepared call result plan from
  `prepare::classify_prepared_call_boundary_move`, prepared after-call ABI
  bindings, destination prepared value homes, and prepared f128 carriers. State
  mutation is limited to `CallBoundaryMoveInstructionRecord` construction or a
  frame-slot store record for stack results. Retained AArch64-local emission is
  ABI result-register parsing/conversion, scalar/FPR/VREG view choice,
  frame-slot memory operand spelling, and selected call-boundary machine
  record emission.
- Preservation and republication lowering:
  `make_callee_saved_preservation_home_republication_instruction`,
  `publish_stack_preserved_call_values`, and before-call prior-preservation
  handling inside `lower_before_call_move` via
  `make_prior_preserved_call_argument_source`.
  `make_call_instruction` also publishes prepared call preserve effects when
  `prepared_call_preserve_effect_publication_enabled()` is on. Consumed facts
  are `PreparedCallPreservedValue`, prepared preservation routes, endpoint
  stack/register data, indexed first stack-preserved values for a call, current
  prepared value homes, and current `BlockScalarLoweringState` emitted
  registers. State mutations are `BlockScalarLoweringState` reads for emitted
  registers, call-boundary move construction for prior-preserved arguments, and
  select-chain materialization records for stack republication. Retained
  AArch64-local emission is callee-saved register spelling, frame-slot store
  lines, preservation-home publication records, and ABI/local effect resources.
- Scalar producer dispatch bridge:
  `lower_scalar_call_argument_producers`,
  `materialize_scalar_call_argument_value`,
  `find_prepared_scalar_call_argument_source_producer`,
  `materialize_direct_global_select_chain_call_argument`,
  `materialize_local_aggregate_address_call_argument`, and
  `materialize_missing_frame_slot_call_arguments`. Consumed facts are the
  idea-116 prepared edge-publication source-producer lookups, direct-global
  select-chain dependencies, call argument plans, prepared value homes,
  prepared move classifications, local aggregate address publication flags,
  and current block BIR producers. State mutations are recursive updates to
  `BlockScalarLoweringState` through `record_emitted_scalar_register`,
  appended lowered dispatch-bridge instructions, and synthetic
  `CallBoundaryMoveInstructionRecord` construction for missing frame-slot
  argument publication. Retained AArch64-local emission is scalar instruction
  lowering, register retargeting for emitted operands, select-chain assembler
  line construction, local aggregate address payloads, and dispatch-bridge
  machine instruction wrapping.
- Result recording and late publication:
  `record_call_result_source_register`,
  `retarget_fpr_call_result_store_value_to_emitted_scalar`,
  `materialize_call_boundary_source_to_destination`,
  `retarget_call_boundary_source_to_emitted_scalar`,
  `record_call_boundary_destination`, and
  `record_call_boundary_source_in_destination`. Consumed facts are
  `PreparedCallPlan::result`, after-call result lane bindings, prepared value
  homes/regalloc, prepared edge-publication source producers, same-block BIR
  producers, idea-117 current-block publication facts, and the live
  `BlockScalarLoweringState`. State mutations are direct
  `BlockScalarLoweringState` updates, retargeting of call-boundary move source
  registers and destination register views, memory-store value replacement for
  FPR/VREG call results, and emitted scalar aliases for source-in-destination
  moves. Retained AArch64-local emission is selected FP/f128/GP materialization
  lines, scratch-register choice, q/vector move spelling, and late
  call-boundary materialization instruction records.
- Ordinary call-boundary move/binding record construction where coupled to the
  deferred clusters:
  `call_boundary_move_selection_status`,
  `call_boundary_abi_binding_selection_status`,
  `make_call_boundary_move_instruction`,
  `make_call_boundary_abi_binding_instruction`,
  `make_call_instruction`,
  `lower_prepared_call_instruction`, and
  `lower_call_instruction`. These consume prepared call plans, ABI bindings,
  move records, call clobber/preserve/result facts, indirect callee facts, and
  outgoing stack area facts. State mutation is record assembly only:
  operands/defs/uses/clobbers/preserves/side effects are populated for the
  selected machine node. Retained AArch64-local emission is call opcode choice,
  direct/indirect callee operand spelling, variadic helper checks,
  memory-return storage handoff to the stack-frame owner, outgoing stack byte
  alignment/base selection, and target machine-origin metadata.

## Step 3 Cluster Disposition Table

| Cluster | Disposition | Evidence and prepared facts | Reject-risk notes |
| --- | --- | --- | --- |
| Before-call move bundle lowering | `ready-local-owner` | A bounded AArch64-local owner can consume `PreparedCallPlan`, `PreparedMoveBundle`, `PreparedCallBoundaryEffectPlan`, `PreparedMoveResolution`, prepared value homes, prepared ABI bindings, prepared source selections, prepared f128 carriers, indexed immediate argument lookups, and idea-114 `PreparedCallPlan::outgoing_stack_argument_area`. The safe boundary is the translation from already-prepared move authority into AArch64 register, memory, immediate, FP, and f128 machine records. | Reject any route that reselects stack homes, recomputes outgoing stack area totals, reopens f128 carrier lookup, or rediscover immediate argument publication. The owner must preserve `x16` base addressing, q-register rendering, inline-asm materialization, and target diagnostics as AArch64-local behavior. |
| After-call result/value lowering | `ready-local-owner` | A local owner can consume the prepared call result plan from `prepare::classify_prepared_call_boundary_move`, prepared after-call ABI bindings, destination prepared value homes, and prepared f128 carriers. The safe boundary is AArch64 result-register/view conversion plus call-boundary move or frame-slot store record construction. | Reject routes that move ABI result-register spelling, scalar/FPR/VREG view selection, frame-slot memory operand spelling, or f128 carrier rendering into shared BIR/prealloc. Do not rederive destination homes or duplicate stack-frame-slot ownership from idea 93. |
| Preservation and republication lowering | `contract-needed` | The cluster has prepared inputs (`PreparedCallPreservedValue`, prepared preservation routes, endpoint stack/register data, indexed first stack-preserved values, current prepared value homes, and current emitted registers), but it crosses before-call prior-preservation moves, call preserve-effect publication, and stack republication records. | Missing visibility: focused dumps/tests should show preserve-effect publication enabled and disabled, prior stack-preserved argument consumption, and stack-to-register republication through the same route. Until that exists, extraction risks hiding a behavior change in preservation side effects or reopening idea-93 stack-preserved source authority. |
| Scalar producer dispatch bridge | `move-forward-needed` | The cluster already consumes idea-116 facts for edge-publication source-producer lookup, direct-global select-chain dependencies, call argument plans, prepared value homes, prepared move classifications, local aggregate address publication flags, and current block BIR producers. | Move first: a target-neutral query/fact for call-argument producer materializability and publication source routing, including direct-global select-chain dependencies and missing frame-slot argument publication needs. Reject an AArch64-only owner that rediscovers same producer classification, extends named-case select-chain matching, or masks gaps in shared dispatch facts. |
| Result recording and late publication | `move-forward-needed` | This cluster consumes `PreparedCallPlan::result`, after-call result lane bindings, prepared value homes/regalloc, prepared edge-publication source producers, same-block BIR producers, idea-117 current-block publication facts, and live `BlockScalarLoweringState`. | Move first: a target-neutral result-publication query that describes when a call result source register, source-in-destination alias, or FPR/VREG store value can be retargeted to an already-emitted scalar. Reject a local-only contraction that rebuilds current-block publication facts, comparison publication routing, or same-block producer discovery inside calls lowering. |
| Ordinary call-boundary move/binding record construction where coupled | `keep-in-calls` | The constructors consume prepared call plans, ABI bindings, move records, call clobber/preserve/result facts, indirect callee facts, and outgoing stack area facts, but their mutation is record assembly for the selected call-boundary or call machine node. | No new owner should extract generic instruction-record construction just to shrink `calls.cpp`. Reject routes that move call opcode choice, direct/indirect callee operand spelling, variadic checks, memory-return handoff, outgoing stack byte alignment/base selection, or machine-origin metadata away from AArch64 calls emission. |

### Step 3 No-New-Idea Rationale

- No new idea should reopen the closed stack-frame-slot, f128-carrier, or
  immediate-publication owners from ideas 93, 94, and 95; the Step 3 actionable
  boundaries only consume those owners.
- No new idea should duplicate idea-114 outgoing stack area authority; the
  before-call local owner can consume the prepared call-level area while
  retaining AArch64 store/base emission.
- No new idea should duplicate idea-116 or idea-117 shared prepared-producer or
  comparison publication contracts; the two `move-forward-needed`
  classifications require target-neutral query/fact movement first, not an
  AArch64-only contraction.
- The ordinary record-construction cluster is `keep-in-calls` where it is pure
  call instruction or ABI record emission. It should only be mentioned by later
  follow-up ideas when coupled to an actionable move/result/preservation
  boundary.

## Suggested Next

Execute Step 5 by preparing the closure readiness package for supervisor
review. Confirm that this audit changed only lifecycle artifacts, list the
generated follow-up ideas and intended proof routes, restate duplicate-work
guardrails, and recommend lifecycle closure for idea 118 if the supervisor
accepts the generated follow-up set.

## Step 4 Follow-Up Ideas Created

- `ideas/open/119_aarch64_calls_before_call_move_bundle_local_owner.md`:
  local AArch64 owner for before-call move bundle lowering. Proof route:
  focused backend/AArch64 coverage for register, stack, immediate, FP/f128,
  and outgoing-stack-area argument movement while consuming ideas 93, 94, 95,
  and 114 as closed authority.
- `ideas/open/120_aarch64_calls_after_call_result_value_local_owner.md`:
  local AArch64 owner for after-call result/value lowering. Proof route:
  focused backend/AArch64 coverage for GP, FP, f128/vector-carrier, and
  frame-slot result paths while consuming prepared result plans, destination
  homes, and f128 carrier facts.
- `ideas/open/121_aarch64_calls_preservation_republication_visibility_contract.md`:
  route-visibility contract for preservation/republication before movement.
  Proof route: focused dumps/tests for preserve-effect publication enabled and
  disabled, prior stack-preserved argument consumption, and stack-to-register
  republication.
- `ideas/open/122_prepared_call_argument_producer_materializability_contract.md`:
  shared prepared query/fact for call-argument producer materializability and
  publication-source routing. Proof route: prepared visibility plus
  backend/AArch64 coverage for scalar producer materialization, direct-global
  select-chain call arguments, local aggregate address arguments, and missing
  frame-slot publication.
- `ideas/open/123_prepared_call_result_late_publication_contract.md`:
  shared prepared query/fact for call-result late publication and retargeting.
  Proof route: prepared visibility plus backend/AArch64 coverage for result
  source-register recording, source-in-destination aliasing, FPR/VREG store
  retargeting, and current-block publication consumption.

## Step 4 No-New-Idea Rationale

- No separate idea was created for the ordinary call-boundary move/binding
  record construction cluster. Step 3 classified it as `keep-in-calls` where
  it is pure call instruction or ABI record assembly. Later follow-ups may
  touch those constructors only when coupled to their bounded
  move/result/preservation owner boundary; a generic record-construction
  extraction would be line-count-only work.
- No idea was created for stack-frame-slot operand ownership. Ideas 119, 120,
  121, and 123 must consume idea 93's closed authority instead of reselecting
  stack homes, stack-preserved sources, endpoint stack storage, or memory
  result frame-slot storage.
- No idea was created for f128 carrier ownership. Ideas 119 and 120 must
  consume idea 94's closed f128 carrier and q-register operand authority.
- No idea was created for immediate scalar argument publication. Idea 119 must
  consume idea 95's closed immediate argument publication owner.
- No idea was created for outgoing stack argument area computation. Idea 119
  must consume idea 114's prepared call-level outgoing stack area and keep
  AArch64 `x16`, stack adjustment/restoration, and store ordering local.
- No idea was created to reopen dispatch prepared producer/current-block or
  select-chain contracts from idea 116. Idea 122 is limited to the calls-side
  consumer gap: call-argument producer materializability and publication-source
  routing.
- No idea was created to reopen comparison fused-compare/current-block
  publication contracts from idea 117. Idea 123 is limited to the calls-side
  consumer gap: result late-publication and retargeting.

## Watchouts

- This route is analysis-only; do not edit implementation, test, or build
  metadata files.
- Do not directly edit `src/backend/mir/aarch64/codegen/calls.cpp`.
- Do not reopen completed work from ideas 93, 94, 95, 114, 116, or 117 without
  new evidence.
- Reject monolithic `calls.cpp` shrink proposals or line-count-only progress.
- The ordinary record constructors are only in scope where they carry the
  deferred clusters' move/result/preservation/call-boundary facts; do not turn
  Step 3 into a generic instruction-record extraction.
- Several mapped helpers intentionally consume closed local owners from ideas
  93, 94, and 95; Step 3 should classify those as duplicate-work guardrails,
  not as invitations to reopen stack-slot, f128-carrier, or immediate
  publication ownership.
- Step 4 follow-up ideas should name concrete reject signals: no line-count-only
  `calls.cpp` shrink, no expectation downgrades, no named-case producer
  shortcuts, and no duplicate ownership of ideas 93, 94, 95, 114, 116, or 117.
- The `contract-needed` preservation/republication slice should ask for
  route-visible dumps/tests before movement, not implementation changes.

## Proof

Docs/analysis-only packet. No build or test proof was required because only
planning artifacts changed (`todo.md` plus new `ideas/open/` files); no
`test_after.log` was generated for this packet.
