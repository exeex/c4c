# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc priority cue activation

# Current Packet

## Just Finished
- extended each semantic-BIR `regalloc` object with instruction-order access
  windows and a `crosses_call_boundary` cue derived from actual BIR load,
  store, and call ordering instead of target heuristics or fake intervals
- kept the regalloc contract target-neutral by pairing those priority cues
  with the existing direct-read/direct-write, addressed-access, and
  call-argument exposure summaries for downstream prepared-BIR consumers
- extended the prepare entry-contract test with a representative
  call-crossing local slot and concrete assertions for both single-point and
  call-spanning regalloc access windows

## Suggested Next
- build on the new access-window summary by deriving target-neutral regalloc
  priority buckets that distinguish single-point value-storage objects from
  call-spanning register candidates without widening into physical assignment
- keep any next priority classification keyed off prepared liveness,
  stack-layout, and explicit BIR instruction structure rather than slot-name
  families or target-specific register rules

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
  -j --output-on-failure -R '^backend_prepare_entry_contract$' 2>&1 | tee
  test_after.log`
- passed and wrote `test_after.log`
