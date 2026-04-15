# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- promoted the semantic-BIR regalloc artifact from raw readiness-only output
  to a first allocator-facing contract by publishing a target-neutral
  `preferred_register_pool` field that maps call-spanning value storage toward
  callee-saved pressure, other register candidates toward caller-saved
  pressure, and fixed-stack storage away from register pools entirely
- extended the prepare entry-contract fixture so single-point, multi-point,
  and call-spanning value-storage objects plus fixed-stack storage prove the
  new pool preference end-to-end alongside the existing readiness/access
  summaries

## Suggested Next
- broaden step-4 validation to `^backend_`, or if execution continues inside
  this bucket, publish the next honest allocator-facing regalloc contract such
  as a spill-pressure or register-eligibility cue sourced from prepared facts
  rather than testcase-shaped inference

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
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available
- if follow-on allocator-facing cues are added, keep them sourced from actual
  BIR access order, liveness contracts, and call-crossing facts rather than
  synthetic interval guesses, target register names, or phi-materialization
  scaffolding that does not exercise a real local-slot access window

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_prepare_entry_contract$' 2>&1 | tee
  test_after.log`
- passed and wrote `test_after.log`
