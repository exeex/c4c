# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- projected deferred binding batch membership back onto each deferred prepared
  regalloc object, so access-window-blocked and coordination-blocked
  single-point candidates now carry explicit per-object batch kind, ordering
  policy, and deferred access/home-slot/sync prerequisite state instead of
  forcing downstream consumers to rejoin object frontier facts with batch
  summaries
- kept that per-object deferred binding contract derived from the existing
  allocation sequence and contention frontier: unobserved single-point objects
  project the access-window deferred batch contract directly, while observed
  single-point objects project the coordination-deferred contract with
  satisfied access-window state plus deferred home-slot readiness
- extended the prepare entry backend fixture and regalloc note text to assert
  the new per-object deferred binding cues without reopening target-ingestion
  work or naming physical registers

## Suggested Next
- keep step-4 work inside prepare by projecting binding-ready batch membership
  back onto each ready prepared object as the same kind of per-object binding
  decision/prerequisite contract, so downstream prepared consumers can read a
  uniform binding contract for both ready and deferred register candidates

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

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
