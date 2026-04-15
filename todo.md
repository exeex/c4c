# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- turned the current semantic-BIR regalloc staging into a concrete per-object
  allocation-state contract on `PreparedRegallocObject`, so each prepared
  object now directly carries its allocation state kind, reservation kind and
  scope, home-slot mode, sync policy, coordination categories, and deferred
  reason without forcing downstream consumers to re-join
  `allocation_sequence`, `reservation_summary`, and `contention_summary`
- kept the new object-level state target-neutral by deriving it entirely from
  existing prepare-owned reservation and contention facts, including deferred
  opportunistic single-point candidates and fixed-stack authoritative objects
- broadened the prepare entry-contract fixture to prove the new object-level
  state across nearby deferred single-point, observed opportunistic,
  across-call, local-reuse, and fixed-stack shapes
- refreshed the regalloc prepare note so the public prepare contract now
  advertises the new object-level allocation-state artifact explicitly

## Suggested Next
- keep step-4 work inside prepare by deriving one more concrete
  per-function/per-object allocation frontier from the new object states:
  identify which register candidates are ready for stable prepared
  home-slot/register binding versus still deferred for access-window or
  coordination reasons, without naming physical registers or drifting into
  target ingestion

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
- preserve the current split between register-candidate reservation decisions
  and fixed-stack authoritative objects; do not backdoor fixed-stack storage
  into the reservation sequence just to make a narrow testcase pass

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
