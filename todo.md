# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- added a compact target-neutral `access_shape` summary to each semantic-BIR
  `regalloc` object so prepared contracts now expose direct-read-only,
  direct-write-only, direct-read-write, addressed-access-only, and
  call-argument-exposure-only patterns alongside the existing access counts
  and first/last access cues
- kept the new summary inside prepare-owned semantics by deriving it from the
  existing explicit BIR access counts instead of target register classes,
  physical assignment, or testcase-shaped matcher logic
- extended the prepare entry-contract test and note assertions with
  representative single-read, write-only, read-write, addressed, and
  call-exposure access-shape checks for the new regalloc cue

## Suggested Next
- build on the published access-shape summary by deciding whether prepared
  regalloc needs an assignment-readiness cue that combines priority bucket,
  call-crossing state, and access shape without inventing fake intervals or
  target register policy before physical assignment work
- keep any next regalloc detail keyed off prepared liveness, stack-layout,
  and explicit BIR instruction structure rather than target-specific register
  classes or testcase-shaped heuristics

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
- if follow-on readiness cues are added, keep them sourced from actual BIR
  access order, liveness contracts, and call-crossing facts rather than
  synthetic interval guesses

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_prepare_entry_contract$' 2>&1 | tee
  test_after.log`
- passed and wrote `test_after.log`
