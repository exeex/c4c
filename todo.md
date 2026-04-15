# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- consumed the existing semantic-BIR regalloc `allocation_sequence` into a
  first-pass prepare-owned reservation decision per register candidate by
  publishing target-neutral `reservation_kind`, `reservation_scope`,
  `home_slot_mode`, and `sync_policy` fields that translate the staged
  call-spanning, local-reuse, and single-point buckets into inspectable
  allocator actions without naming physical registers or inventing
  interference graphs
- broadened the prepare entry-contract fixture so nearby call-spanning,
  local-reuse, single-point, and fixed-stack shapes prove the new first-pass
  reservation decision alongside the existing stage labels, ordering, and
  fixed-stack exclusion from the register-candidate sequence

## Suggested Next
- if execution stays inside this bucket, consume the new first-pass
  reservation decisions into a prepare-owned per-function reservation pressure
  or collision summary so regalloc starts exposing where the current prepared
  route would need contention handling next, still without naming target
  registers, synthetic live intervals, or placeholder interference graphs

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
- preserve the current split between register-candidate reservation decisions
  and fixed-stack objects; do not backdoor fixed-stack storage into the
  reservation sequence just to make a narrow testcase pass

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
