# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- extended the ready-only regalloc batch contract with explicit per-batch
  stable-home-slot and sync-handoff prerequisite fields, so prepared consumers
  can read the current binding-ready frontier as concrete batch-level handoff
  facts instead of inferring them from contention summaries alone
- kept the new prerequisite surface derived from the existing
  reservation/contention artifacts: call-boundary batches advertise satisfied
  `stable_home_slot_required` plus ready mixed sync coordination, and
  local-reuse batches advertise satisfied `stable_home_slot_preferred` plus
  ready mixed sync coordination, without naming physical registers or widening
  into target-ingestion work
- proved the new batch prerequisite contract in the prepare entry fixture and
  reran the delegated backend subset successfully

## Suggested Next
- keep step-4 work inside prepare by projecting the new batch prerequisite
  state down into explicit per-binding handoff notes for the binding-ready
  frontier, so downstream prepared consumers can tell which individual binding
  decisions inherit stable-home-slot and sync-ready guarantees from their batch
  without backfilling deferred single-point cases or naming target registers

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
