# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- consumed the existing semantic-BIR regalloc contract in a first prepare-
  owned allocator decision by publishing a per-function register-candidate
  `allocation_sequence` that stages candidates as `stabilize_across_calls`,
  `stabilize_local_reuse`, or `opportunistic_single_point` and orders them
  from the current readiness and eviction-friction contracts rather than
  adding another parallel hint
- broadened the prepare entry-contract fixture so nearby call-spanning,
  local-reuse, single-point, and fixed-stack shapes prove the new allocation-
  sequence decision, including stage labels, ordering, and exclusion of
  fixed-stack storage from the register-candidate sequence

## Suggested Next
- if execution stays inside this bucket, consume `allocation_sequence` in the
  next prepare-owned allocator pass step so regalloc starts producing an
  inspectable first-pass reservation or assignment attempt from that sequence
  without naming target registers, synthetic live intervals, or placeholder
  interference graphs

## Watchouts
- do not let the current regalloc packet drift into target ingestion work that
  belongs to `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push prepare ownership back into semantic lowering or target codegen
- keep downstream regalloc contracts driven by prepared liveness and
  stack-layout ownership rather than target-specific rules or slot-name pattern
  matching
- if more source kinds appear, classify them from explicit semantic ABI facts,
  intrinsic identity, or lowering metadata instead of inventing name-based
  buckets inside liveness/regalloc
- keep the regalloc artifact inspectable and target-neutral; prefer concrete
  access summaries over broad notes, but do not fake live ranges or
  interference until the data is really available
- keep any future allocator-facing hints semantically distinct from the
  existing priority bucket, preferred pool, spill-pressure, and readiness
  fields so the prepared contract does not devolve into renamed duplicates
- if follow-on allocator-facing cues are added, keep them sourced from actual
  BIR access order, liveness contracts, and call-crossing facts rather than
  synthetic interval guesses, target register names, or phi-materialization
  scaffolding that does not exercise a real local-slot access window
- keep the new spill/restore-locality cue tied to observable access-window
  width and fixed-stack exposure kind; do not let it collapse back into a
  renamed reload-cost or preferred-pool field
- keep the new register-eligibility cue semantically distinct from priority,
  locality, and readiness: it should answer whether the current prepared facts
  still permit a register-resident strategy, not which pool or window shape is
  preferred once that strategy is chosen
- keep the new spill-sync cue focused on synchronization direction between a
  register-resident strategy and its storage home; do not let it collapse into
  another reload-cost, locality, or eligibility synonym
- keep the new home-slot-stability cue focused on whether the current
  prepared facts imply a stable stack home for the object; do not let it
  collapse into another spill-sync or locality synonym
- keep `allocation_sequence` as a prepare-owned decision that consumes the
  existing contract; do not let it devolve into another renamed hint list or a
  target-specific register assignment table
- if a follow-on packet starts emitting reservation or assignment attempts,
  keep them target-neutral and derived from the staged sequence plus current
  prepared stack/liveness facts rather than from synthetic intervals or
  placeholder interference graphs

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
