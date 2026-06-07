# Current Packet

Status: Active
Source Idea Path: ideas/open/123_prepared_call_result_late_publication_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Characterize Existing Call-Result Late-Publication Routes

## Just Finished

Completed plan Step 1: characterized the existing call-result
late-publication route cluster before adding any shared query.

Cluster map:
- `lower_after_call_moves` is the after-call entry point. It lowers explicit
  after-call move-bundle entries, synthesizes a result-to-stack publication
  when `PreparedCallResultPlan` says the ABI result register must populate a
  stack home and no after-call bundle already publishes it, then emits
  preservation republication effects.
- `lower_after_call_move` gathers the prepared destination home, prepared
  call-boundary move classification, and destination f128 carrier facts, then
  delegates concrete selected emission to `AfterCallMoveLocalOwner`.
- `AfterCallMoveLocalOwner::instruction` currently owns the local selected
  result move cases: register-to-register GPR/FPR/VREG, stack-home
  publication from ABI GPR/FPR/VREG result registers, and FPR placement
  fallback for after-call result moves. It consumes `PreparedCallResultPlan`,
  `PreparedCallBoundaryMoveClassification`, ABI binding data, value homes, and
  f128 carrier facts.
- `record_call_result_source_register` seeds `BlockScalarLoweringState` from
  the prepared call result source register when the result has a destination
  value home, and also records after-call result lane bindings via
  `find_prepared_after_call_result_lane_binding`.
- `materialize_call_boundary_source_to_destination` performs late publication
  from an already-known call-boundary source into a destination register. It
  handles aggregate register lanes, FPR/VREG call-argument publications,
  frame-slot-backed source values, and GP source materialization through
  prepared source-producer facts plus target-local scratch/register emission.
- `retarget_call_boundary_source_to_emitted_scalar` rewrites a
  `CallBoundaryMoveInstructionRecord` source from memory/register to an
  already-emitted scalar register by value name or value id, including a
  floating-register fallback by value id.
- `record_call_boundary_destination` records any selected destination register
  into scalar state.
- `record_call_boundary_source_in_destination` records an alias saying the
  source value now resides in the selected destination register when the move
  source was a prepared memory/register value.
- `retarget_fpr_call_result_store_value_to_emitted_scalar` retargets a
  selected local store value to an already-emitted CallAbi FPR/VREG scalar
  when the store value is an F32/F64/F128 named value and the store record
  matches that type.

Target-neutral prepared decisions already present:
- `PreparedCallResultPlan` carries result value bank, source/destination
  storage kinds, destination value id, ABI source register name/bank/placement,
  destination register or stack-home facts, and stack offset.
- `classify_prepared_call_boundary_move` already classifies after-call
  `CallResultAbi` moves against the prepared result plan and ABI binding,
  including missing/mismatched-result statuses.
- `PreparedAfterCallResultLaneBinding` lookups expose lane-to-ABI binding
  facts for result lane source-register recording.
- `PreparedCallBoundaryEffectPlan` exposes after-call explicit moves and
  preservation republication ordering without requiring AArch64 to recompute
  effect sequencing.
- Existing prepared source-producer/current-block publication facts can answer
  producer/publication availability; Step 2 should consume those facts instead
  of adding same-block producer discovery in AArch64.

AArch64-local behavior to keep local:
- Register parsing/conversion, expected register view selection, FPR/VREG/q
  spelling, scratch-register choice, emitted scalar-state mutation, memory
  store record construction, call-boundary machine-record construction, f128
  carrier transport records, and concrete FP/f128/GP materialization lines.
- `materialize_call_boundary_source_to_destination` still performs target-local
  emission tracing for concrete byte/lane/FP/F128 materialization; shared code
  should not take over those instruction payload details.
- `retarget_fpr_call_result_store_value_to_emitted_scalar` should keep the
  actual `MemoryInstructionRecord` mutation and AArch64 register-bank/view
  checks local even after a shared query identifies the retargeting need.

Existing visibility surfaces:
- Prepared printer call-plan output already shows result source/destination
  storage, ABI source register facts, destination register/slot, and stack
  offsets under `--- prepared-call-plans ---`.
- Prepared value-location printer shows after-call move bundles and ABI
  bindings; `backend_prealloc_call_boundary_classification` covers call-result
  classification and missing/mismatched statuses.
- `backend_call_boundary_effect_plan` covers after-call explicit move effect
  ordering and preservation republication neighbors.
- `backend_prepare_frame_stack_call_contract` covers prepared call-result ABI
  bindings, result move structure, float-call result ownership, and
  stack-result slot contracts.
- `backend_aarch64_call_boundary_owner` covers scalar, FPR, f128 stack-home,
  and f128 register-home after-call result publication.
- `backend_aarch64_instruction_dispatch` covers dispatch-level after-call
  result ordering, prepared register result moves, HFA lane moves, and HFA
  result stores consuming ABI result lanes.
- `backend_aarch64_target_instruction_records` covers selected after-call
  result move records and neighboring f128 call-boundary record shape.

Route gaps for Step 2:
- No single shared prepared query currently answers "this call-result source
  can be late-published/aliased/retargeted now"; AArch64 combines prepared
  result plans, emitted scalar state, call-boundary records, and local
  source-value tracing.
- Source-in-destination aliasing and FPR/VREG store-value retargeting are
  currently implicit in AArch64 scalar-state helpers rather than visible as a
  prepared fact or printer field.
- Current-block publication consumption must be based on existing idea 117
  facts; do not reintroduce comparison/current-block publication discovery in
  calls lowering.
- Same-block producer traces inside emission helpers remain target-local for
  now, but Step 2 should identify which target-neutral availability result can
  be exposed without moving FP/f128/GP instruction emission.

## Suggested Next

Execute Step 2: add a shared call-result late-publication fact or query that
describes target-neutral availability for result source-register publication,
source-in-destination aliasing, FPR/VREG store-value retargeting, and
current-block publication consumption. Recommended focused proof subset for
the first code-changing packet:
`ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prealloc_call_boundary_classification|backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records)$'`.

## Watchouts

- Do not reopen idea 117 fused-compare, materialized-compare, or
  current-block publication authority.
- Do not reopen idea 116 prepared producer or same-block producer authority.
- Do not recompute destination homes or stack storage covered by idea 93.
- Do not move AArch64 FP/f128/GP materialization lines, scratch-register
  policy, q/vector move spelling, memory-store record emission, or
  call-boundary machine records into shared code.
- Treat expectation downgrades, unsupported-path rewrites, and renamed wrappers
  around the old AArch64 retargeting logic as route failures.

## Proof

No build/test run. This was a characterization-only `todo.md` update.
Read-only probes used:
- `git status --short`
- `sed` on `plan.md`, `todo.md`, and
  `ideas/open/123_prepared_call_result_late_publication_contract.md`
- `c4c-clang-tool-ccdb find-definition` for the six delegated
  `calls.cpp` helper symbols
- targeted `rg`/`sed` over `calls.cpp`, prepared lookup/printer files, and
  focused backend tests
