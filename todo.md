# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- projected the existing ready-only stable-binding pass contract back onto each
  ready binding entry, so downstream prepared consumers can walk from
  call-boundary and local-reuse pass order to per-binding prerequisite state
  without consulting batch summaries
- kept the earlier ready/deferred batches, unified handoff summaries, and
  stable-binding pass summaries in place, but extended the binding decision
  surface with prepare-owned pass order plus covered binding-order span for the
  active ready batches only
- extended the prepare entry backend fixture and regalloc note text to assert
  the new per-binding stable-pass cues alongside the existing binding-order,
  prerequisite, and ready-only pass-summary contract without reopening target
  ingestion work or flattening deferred frontiers

## Suggested Next
- keep step-4 work inside prepare by projecting the same ready-only stable-pass
  contract onto the ready prepared objects themselves, so downstream consumers
  can start from a ready object and recover both its stable pass slot and local
  prerequisite envelope without consulting binding batches or handoff summaries

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
- keep the new per-binding stable-pass metadata derived from the existing
  ready-only pass summaries; do not fork a second pass-order model on the
  binding entries or backfill deferred bindings with fake ready pass slots

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
