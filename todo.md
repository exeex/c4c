# Current Packet

Status: Active
Source Idea Path: ideas/open/122_prepared_call_argument_producer_materializability_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Shared Producer Materializability Fact Or Query

## Just Finished

Completed plan Step 1: characterized the existing call-argument producer
materialization and publication route cluster before moving authority.

Cluster map:
- `dispatch_prepared_block` calls `lower_scalar_call_argument_producers` before
  before-call moves, then retargets those moves from emitted scalar registers,
  and later calls `materialize_missing_frame_slot_call_arguments` before the
  actual call instruction.
- `lower_scalar_call_argument_producers` skips inline variadic helper calls,
  obtains prepared edge-publication source-producer lookups from
  `context.function.prepared_lookups` or builds a fallback lookup, finds each
  `PreparedCallArgumentPlan`, first tries
  `materialize_direct_global_select_chain_call_argument`, then passes
  local-aggregate-address eligibility and producer lookups into
  `materialize_scalar_call_argument_value`.
- `find_prepared_scalar_call_argument_source_producer` consumes indexed
  `PreparedEdgePublicationSourceProducerLookups`, but still filters locally for
  named values in the current control-flow block and producer instruction
  order before the call.
- `materialize_scalar_call_argument_value` owns the current materializability
  decision: local aggregate address publication, already-emitted checks,
  fallback prepared result-register recording, `LoadLocal` acceptance,
  binary-producer opcode allowlisting through
  `is_scalar_call_argument_producer_opcode`, recursive operand readiness, and
  AArch64 `lower_scalar_instruction` emission plus register retargeting.
- `materialize_direct_global_select_chain_call_argument` already consumes the
  prepared call-argument direct-global select-chain dependency, but still owns
  value-home checks, scratch selection, FP/GPR register view policy, select
  chain line emission, and emitted-register recording.
- `materialize_missing_frame_slot_call_arguments` walks prepared call-plan
  arguments and prepared value homes to decide which frame-slot GPR arguments
  still need a publication move; it consumes source-selection facts and
  before-call move classification, then emits AArch64 call-boundary move records.

Boundary classification:
- Target-neutral/prepared-owned candidates: whether a call-argument source
  producer is materializable for call lowering; the producer kind, block, and
  instruction identity; source publication routing; direct-global select-chain
  dependency availability; missing frame-slot argument publication need and
  source-selection facts.
- AArch64-local details: scalar instruction spelling, recursive emission order
  mechanics, emitted-register table updates, destination register retargeting,
  scratch-register selection, FP/GPR register view conversion, select-chain
  assembler lines and synthetic labels, local aggregate address payload
  construction, and machine instruction wrapping.

Existing visibility surfaces:
- `PreparedCallArgumentPlan` already prints `arg.source_selection=*` and
  `direct_global_select_chain=yes` through the prepared call-plan printer.
- Prepared select-chain printer rows already show select-chain materialization
  roots, direct-global dependency facts, and source-producer kind/block/inst.
- `backend_prepare_frame_stack_call_contract` covers call-argument source
  shape, direct-global select-chain dependency lookup, prior preservation
  source selection, local/derived frame address selection, byval lane source
  selection, and missing local aggregate frame-slot address source selection.
- `backend_aarch64_instruction_dispatch` covers direct dispatch behavior for
  scalar producers, select-chain materialization, direct and missing local
  aggregate address arguments, and missing frame-slot argument materialization.

Route gaps to close in later steps:
- No shared prepared query currently answers "this call-argument source
  producer is materializable for call lowering"; AArch64 still recomputes the
  binary-op allowlist and source-producer safety gates.
- Publication-source routing is visible on prepared call plans and select-chain
  dumps, but there is not yet a single call-argument producer fact combining
  producer materializability with publication source.
- Direct-global select-chain dependency is a prepared fact, but AArch64 still
  decides nearby materialization viability and target register/scratch policy.
- Missing frame-slot argument publication uses prepared source selection and
  move classification, but the target-neutral "needs a synthetic frame-slot
  argument publication" decision is still embedded in AArch64 iteration and
  filters.
- Local aggregate address payloads should remain explicitly target-local even
  when the shared surface exposes the source-selection need.

## Suggested Next

Execute Step 2: add or extend a shared prepared query/fact for call-argument
source-producer materializability. Keep AArch64 as a consumer only except for a
minimal shim if needed, and add printer or route-test visibility for ordinary
scalar producer routes.

## Watchouts

- Do not reopen idea 116 producer, current-block publication, join-routing, or
  select-chain authority.
- Do not add AArch64 same-block producer scans, BIR-name matching, or
  direct-global/select-chain named-case shortcuts.
- Do not move AArch64 scalar instruction spelling, register retargeting,
  select-chain assembler lines, local aggregate address payloads, or machine
  instruction wrapping into shared code.
- Treat expectation downgrades, unsupported-path rewrites, and renamed wrappers
  around the old AArch64 rediscovery logic as route failures.
- Recommended focused proof subset for code-changing packets:
  `ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_prepared_memory_operand_records)$'`.
- If Step 2 touches prepared printer/dump behavior, ensure the proof checks
  call-plan and select-chain visibility, not just AArch64 route assembly.

## Proof

No build or test command was run. This was a docs/analysis-only packet and only
`todo.md` changed, so no build/test proof or `test_after.log` was required.
