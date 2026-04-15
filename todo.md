# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc contract activation

# Current Packet

## Just Finished
- enriched each semantic-BIR `regalloc` object with explicit direct-read,
  direct-write, addressed-access, and call-argument exposure counts derived
  from actual BIR loads, stores, and calls instead of slot-name heuristics
- kept the regalloc contract target-neutral by publishing those access anchors
  alongside the existing `register_candidate` vs `fixed_stack_storage`
  classification for downstream prepared-BIR consumers
- extended the prepare entry-contract test so the semantic prepare route now
  checks concrete access/exposure summaries for representative local-slot,
  address-exposed, and call-result storage objects

## Suggested Next
- build on the new access summary by publishing phase-order-aware regalloc
  priority cues such as call-crossing or def/use ordering that later physical
  assignment can consume without reconstructing liveness from scratch
- keep any next regalloc detail keyed off prepared liveness, stack-layout, and
  explicit BIR structure rather than slot-name families or target heuristics

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

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
