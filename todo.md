# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 prepared liveness ownership reset

# Current Packet

## Just Finished
- grew `PreparedLiveness*` so liveness now publishes function-level call-point
  metadata and object-level access-shape, direct read/write, addressed-access,
  call-argument exposure, instruction-order access-window, and call-crossing
  cues without changing prepared IR
- implemented the new prepared liveness analysis in
  `src/backend/prepare/liveness.cpp` as an information-only phase over
  prepared stack objects, keeping responsibility aligned with
  `ref/claudes-c-compiler/src/backend/liveness.rs`
- extended `backend_prepare_entry_contract_test` to lock the new liveness
  contract at both function and object granularity, including call points and
  representative object access windows
- removed the duplicate regalloc-local access-summary rebuild path from
  `src/backend/prepare/regalloc.cpp` so regalloc now consumes
  `PreparedLivenessObject` access-shape, access-window, and call-crossing
  facts instead of rescanning BIR instructions for the same data

## Suggested Next
- take the next packet in `src/backend/prepare/regalloc.cpp` to compress or
  delete policy scaffolding that still exists only to inflate the prepared
  contract beyond the current reference-shaped responsibility split
- keep that follow-on cleanup focused on reducing regalloc surface area while
  preserving the now-correct `liveness -> regalloc` dependency direction

## Watchouts
- do not let the current regalloc packet drift into target ingestion work that
  belongs to `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push prepare ownership back into semantic lowering or target codegen
- keep downstream regalloc contracts driven by prepared liveness and
  stack-layout ownership rather than target-specific rules or slot-name pattern
  matching
- do not add more liveness-like fact gathering to
  `src/backend/prepare/regalloc.cpp` until the missing prepared liveness
  ownership is established in `src/backend/prepare/liveness.cpp`
- treat `ref/claudes-c-compiler/src/backend/liveness.rs` and
  `ref/claudes-c-compiler/src/backend/regalloc.rs` as the responsibility split:
  liveness owns analysis facts first, regalloc consumes them second
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
- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^backend_prepare_entry_contract$' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
