# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- projected the existing ready-only batch prerequisite and handoff state down
  into each `binding_ready` regalloc binding decision, so downstream prepared
  consumers can read per-binding stable-home-slot and sync-ready guarantees
  without reconstructing them from batch summaries alone
- kept the new per-binding fields derived directly from the current
  reservation/contention frontier: call-boundary bindings inherit satisfied
  `stable_home_slot_required` plus ready mixed sync coordination, and
  local-reuse bindings inherit satisfied `stable_home_slot_preferred` plus
  ready mixed sync coordination, without naming target registers or widening
  into target-ingestion work
- extended the prepare entry backend fixture to assert the new per-binding
  prerequisite/handoff contract for the current binding-ready frontier

## Suggested Next
- keep step-4 work inside prepare by projecting the current binding-batch
  ordering policy down into each `binding_ready` decision, so downstream
  prepared consumers can read the per-binding sequencing contract without
  consulting batch summaries or backfilling deferred single-point cases

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
- keep any follow-on binding prerequisite or handoff work scoped to the
  current `binding_ready` batches and derived from the existing reservation
  plus contention summaries; do not backdoor single-point deferred cases into
  a fake stable-binding path just to flatten the frontier counts
- preserve the current split between register-candidate reservation decisions
  and fixed-stack authoritative objects; do not backdoor fixed-stack storage
  into the reservation sequence just to make a narrow testcase pass

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
