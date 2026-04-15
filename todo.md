# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- extended the semantic-BIR regalloc artifact with a target-neutral
  `reload_cost_hint` field sourced from current prepared facts: single-use
  reads stay light, call-spanning reads stay expensive, repeated read windows
  advertise amortized reload reuse, write-only values stay reload-free, and
  fixed-stack exposure splits into address-exposed versus call-exposed cases
- broadened the prepare entry-contract fixture so nearby single-point,
  multi-point, call-spanning, write-only, address-exposed, and call-exposed
  shapes all prove the new reload-cost cue alongside the existing
  pool/pressure/readiness/access summaries

## Suggested Next
- if execution stays inside this bucket, publish one more allocator-facing
  regalloc cue that still comes directly from prepared facts, such as a
  register-eligibility or materialization-timing hint keyed by exposure kind
  and first/last access order rather than target names or synthetic intervals

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

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
