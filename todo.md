# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- turned the existing ready-vs-deferred frontier into a concrete ready-only
  binding artifact by adding `binding_sequence` and `binding_batches` on
  `PreparedRegallocFunction`, so binding-ready register candidates now carry a
  prepare-owned batch kind plus per-batch order derived from the current
  reservation/contention summaries without naming physical registers
- kept deferred single-point candidates parked outside the new binding
  sequence, so the artifact only covers the current binding-ready across-call
  and local-reuse frontier instead of flattening deferred cases into fake
  stable bindings
- proved the new batch/order artifact on nearby backend shapes in the prepare
  contract fixture: call-spanning candidates form one ordered
  `call_boundary_binding_batch`, local-reuse candidates form one ordered
  `local_reuse_binding_batch`, and deferred single-point candidates stay out
  of the binding sequence
- refreshed the regalloc prepare note so the public prepare contract now
  advertises the ready-only binding batch/order artifact alongside the
  existing binding frontier

## Suggested Next
- keep step-4 work inside prepare by turning the new ready-only binding
  batches into explicit per-batch binding prerequisites for stable home-slot
  and sync handoff, so downstream prepared consumers can see which batch-level
  coordination facts are already satisfied versus still deferred, while still
  avoiding physical-register naming or target-ingestion work

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
