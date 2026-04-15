# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- projected the ready binding-handoff summary contract back onto prepared
  regalloc objects by recording each ready object's handoff allocation stage
  and candidate count directly from the existing handoff summaries instead of
  making downstream consumers look through binding-batch or stable-pass
  summaries to recover the full ready object handoff
- kept that new object-level handoff surface derived from the existing
  `binding_handoff_summary` records rather than inventing a parallel ready
  sequencing model or recomputing counts from object-local state
- extended the backend prepare entry fixture to assert the new object-level
  ready handoff stage/count surface across the representative call-boundary
  and local-reuse ready objects alongside the previously added pass-order and
  prerequisite cues

## Suggested Next
- keep step-4 work inside prepare by projecting the same handoff-summary
  stage/count contract onto deferred prepared regalloc objects, so downstream
  consumers can recover the explicit deferred frontier handoff per object
  without consulting deferred binding-batch summaries

## Watchouts
- do not let the current regalloc packet drift into target ingestion work that
  belongs to `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push prepare ownership back into semantic lowering or target codegen
- keep downstream regalloc contracts driven by prepared liveness and
  stack-layout ownership rather than target-specific rules or slot-name pattern
  matching
- keep the regalloc artifact inspectable and target-neutral; prefer concrete
  access summaries over broad notes, but do not fake live ranges or
  interference until the data is really available
- if more source kinds appear, classify them from explicit semantic ABI facts,
  intrinsic identity, or lowering metadata instead of inventing name-based
  buckets inside liveness/regalloc
- keep the new object-level allocation state as a projection of the existing
  staged reservation/contention artifacts; do not fork a second allocator
  ordering model or let the per-object state drift from the sequence/summaries
- keep future ready/deferred frontier work derived from explicit prepared
  access-window, sync, home-slot, and contention facts rather than from target
  register names, synthetic intervals, or placeholder interference graphs
- keep any follow-on deferred-batch or per-binding work scoped to the current
  opportunistic-single-point frontier and derived from existing reservation,
  contention, and object-access facts; do not backdoor deferred single-point
  cases into a fake stable-binding path just to flatten the frontier counts
- preserve the current split between register-candidate reservation decisions
  and fixed-stack authoritative objects; do not backdoor fixed-stack storage
  into the reservation sequence just to make a narrow testcase pass
- keep the new stable-binding pass surface derived from ready handoff
  summaries and current binding order only; do not invent global binding
  indices, target register names, or fake deferred pass ordering just to make
  downstream consumption look uniform
- keep the new object-level ready handoff stage/count surface derived from the
  existing handoff summaries; do not fork a second summary model on objects,
  and do not backfill deferred entries with fake ready-stage or ready-count
  values just to flatten the frontier contract

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
