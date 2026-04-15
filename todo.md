# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- proved that the object-level handoff projection already covers deferred
  prepared regalloc objects as well as ready ones by asserting the deferred
  frontier handoff allocation stage and candidate count on representative
  access-window-deferred and coordination-deferred objects
- kept the deferred object-level handoff surface derived from the existing
  `binding_handoff_summary` records rather than inventing a second deferred
  summary model or recomputing batch counts from object-local state
- extended the backend prepare entry fixture so both representative deferred
  frontier shapes now assert their explicit per-object handoff stage/count
  alongside the previously added deferred prerequisite and uniform binding
  contract cues

## Suggested Next
- checkpoint the current step-4 object-first regalloc frontier contract and
  choose the next prepare-owned packet from the remaining regalloc consumer
  gaps, because the ready and deferred handoff stage/count projection work is
  now covered on representative shapes

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
- keep the same object-level deferred handoff stage/count surface derived from
  the existing deferred handoff summaries; do not invent separate deferred
  batch counters or object-local recount logic just to make the frontier look
  more uniform than the current prepare facts support

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
