# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- extended prepare-owned semantic-BIR regalloc with a per-function
  `reservation_summary` artifact that folds the existing
  `allocation_sequence` together with current prepared access-window,
  home-slot, and sync-policy facts into inspectable bucket summaries for
  `stabilize_across_calls`, `stabilize_local_reuse`, and
  `opportunistic_single_point`
- published target-neutral pressure/collision signals plus bucket counts for
  overlapping windows, unobserved windows, reservation scopes, home-slot
  requirements, and restore/writeback/bidirectional sync mixes without naming
  physical registers or inventing interference graphs
- broadened the prepare entry-contract fixture so nearby call-spanning,
  local-reuse, opportunistic single-point, and fixed-stack shapes prove the
  new per-function summary alongside the existing per-object reservation
  decisions and register-candidate ordering
- added a second per-function `contention_summary` reduction that groups the
  existing reservation summaries into explicit follow-up categories for
  window coordination, sync coordination, and home-slot handling so later
  prepared-BIR consumers do not need to reverse-engineer raw bucket counts

## Suggested Next
- keep step-4 work inside prepare by deciding whether liveness/regalloc should
  now publish one more object-level artifact that explains why unobserved
  single-point candidates remain deferred, or whether the next honest slice is
  to stop expanding summaries and start turning these contracts into a more
  concrete prepared-BIR allocation state that target ingestion can eventually
  consume without raw-LIR assumptions

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
- keep future reservation-pressure or collision summaries derived from the
  staged sequence plus current prepared access-window, sync, and home-slot
  facts rather than from target register names, synthetic intervals, or
  placeholder interference graphs
- keep the new first-pass reservation fields semantically distinct from the
  existing priority bucket, preferred pool, spill-pressure, and readiness
  hints: they should express allocator action, not just restate earlier cues
- keep `reservation_summary` aligned with the existing ordered
  `allocation_sequence`; do not fork a separate ordering model or sneak in
  pairwise pseudo-interference state
- preserve the current split between register-candidate reservation decisions
  and fixed-stack objects; do not backdoor fixed-stack storage into the
  reservation sequence just to make a narrow testcase pass

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
