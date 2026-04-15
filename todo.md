# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- extended the semantic-BIR regalloc artifact with a target-neutral
  `spill_sync_hint` field sourced from current prepared facts: read-only
  values advertise restore-only sync, write-only values advertise writeback-
  only sync, read/write windows advertise bidirectional sync, and fixed-stack
  storage stays memory- or call-boundary-authoritative based on its prepared
  exposure contract
- broadened the prepare entry-contract fixture so nearby single-point,
  adjacent multi-point, call-spanning, write-only, address-exposed, and
  call-exposed shapes all prove the new spill-sync cue alongside the existing
  pool/pressure/reload/materialization/locality/eligibility/readiness/access
  summaries

## Suggested Next
- if execution stays inside this bucket, publish one more allocator-facing
  regalloc cue that still comes directly from prepared facts, such as a
  home-slot stability hint keyed by access-window width plus read/write
  direction rather than target names, synthetic intervals, or placeholder
  interference claims

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

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
