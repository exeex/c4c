# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- derived a concrete ready-vs-deferred prepared binding frontier from the
  existing object-level regalloc state, so each `PreparedRegallocObject` now
  carries `binding_frontier_kind` and `binding_frontier_reason` without
  inventing physical registers, live intervals, or target-side legality
- added per-function frontier counts on `PreparedRegallocFunction` to make the
  current stable-binding-ready versus deferred-for-access-window versus
  deferred-for-coordination split directly inspectable from prepare-owned
  artifacts
- proved the frontier on nearby same-shape cases in the backend prepare entry
  contract fixture: unobserved opportunistic objects stay deferred for missing
  access windows, observed batched single-point cases stay deferred for
  coordination, across-call and local-reuse candidates become binding-ready,
  and fixed-stack authoritative objects stay outside the register-binding
  frontier
- refreshed the regalloc prepare note so the public prepare contract now
  advertises the ready-vs-deferred binding frontier explicitly

## Suggested Next
- keep step-4 work inside prepare by turning the new ready frontier into a
  concrete prepared binding batch/order artifact for the binding-ready
  register candidates, while leaving deferred single-point cases parked behind
  the current access-window and coordination guards and still avoiding any
  physical-register naming or target-ingestion work

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
- keep any follow-on binding batch/order artifact scoped to the current
  `binding_ready` objects and derived from the existing reservation plus
  contention summaries; do not backdoor single-point deferred cases into a
  fake stable-binding path just to flatten the frontier counts
- preserve the current split between register-candidate reservation decisions
  and fixed-stack authoritative objects; do not backdoor fixed-stack storage
  into the reservation sequence just to make a narrow testcase pass

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
