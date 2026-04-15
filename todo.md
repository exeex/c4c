# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- extended the semantic-BIR regalloc artifact with a target-neutral
  `home_slot_stability_hint` field sourced from current prepared facts:
  single-use reads/writes stay definition- or use-point local, adjacent local
  windows stay stable by direction, call-spanning windows advertise
  call-preserved homes by direction, and fixed-stack storage stays anchored by
  memory or call-boundary exposure contract
- broadened the prepare entry-contract fixture so nearby single-point,
  adjacent multi-point, call-spanning, write-only, address-exposed, and
  call-exposed shapes all prove the new home-slot-stability cue alongside the
  existing pool/pressure/reload/materialization/locality/eligibility/
  readiness/sync/access summaries
- extended the semantic-BIR regalloc artifact with a target-neutral
  `eviction_friction_hint` field sourced from current prepared facts:
  single-point reads/writes stay light to evict, local multi-point windows
  advertise buffered read/write friction, call-spanning windows advertise
  guarded reload/writeback or heavy sync friction by direction, and fixed-stack
  storage stays anchored by memory or call-boundary exposure contract
- broadened the prepare entry-contract fixture so nearby single-point,
  adjacent multi-point, call-spanning, address-exposed, and call-exposed
  shapes all prove the new eviction-friction cue alongside the existing
  pool/pressure/reload/materialization/locality/eligibility/readiness/sync/
  home-slot/access summaries

## Suggested Next
- if execution stays inside this bucket, either tighten the new allocator-
  facing cues into a smaller shared classifier so adjacent fields do not drift
  into renamed duplicates, or start consuming the prepared regalloc contract in
  the first downstream prepare-owned allocator decision without naming target
  registers, synthetic live intervals, or placeholder interference graphs

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
- keep the new eviction-friction cue focused on how disruptive it would be to
  evict a register-resident strategy from the current prepared access window;
  do not let it collapse into another spill-pressure, reload-cost, spill-sync,
  or locality synonym

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
