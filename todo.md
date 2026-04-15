# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- projected the existing ready-only stable-binding pass contract onto ready
  prepared regalloc objects as well as binding entries, so downstream prepared
  consumers can start from a ready object and recover prepare-owned pass
  order/span plus local prerequisite state without consulting batch summaries
  or handoff summaries
- kept the ready-only stable-binding pass summaries as the single ordering
  source by matching ready objects through their existing local source
  contract and binding batch kind instead of inventing deferred pass order or
  a second sequencing model
- extended the backend prepare entry fixture to assert the new object-level
  stable-pass surface on representative call-boundary and local-reuse ready
  objects alongside the existing binding-level contract

## Suggested Next
- keep step-4 work inside prepare by extending the object-first stable-binding
  pass surface across the remaining representative ready regalloc objects so
  downstream consumers can rely on ready-object traversal without consulting
  binding batches

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
- keep the new object-level and per-binding stable-pass metadata derived from
  the existing ready-only pass summaries; do not fork a second pass-order
  model on objects or bindings, and do not backfill deferred entries with fake
  ready pass slots

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
