# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- added a target-neutral `assignment_readiness` cue to each semantic-BIR
  `regalloc` object so prepared contracts now combine the existing priority
  bucket and access-shape summaries into explicit single-point,
  multi-point, call-spanning, or fixed-stack readiness labels without
  inventing target register policy or fake live intervals
- kept the readiness cue prepare-owned by deriving it only from prepared
  liveness contracts, BIR instruction-order access facts, and fixed-stack vs
  register-candidate allocation kind
- extended the prepare entry-contract test and note assertions with
  representative single-read, write-only, call-spanning read/write,
  addressed-storage, and call-exposure readiness checks for the new regalloc
  cue

## Suggested Next
- add one representative non-call-spanning multi-point value-storage case to
  the prepare entry-contract fixture so the new readiness cue is also proved
  on a genuine `multi_point_*_candidate` path instead of only single-point,
  call-spanning, and fixed-stack shapes
- keep that follow-on coverage derived from actual prepared liveness and BIR
  instruction order, not synthetic intervals, target register classes, or
  testcase-shaped heuristics

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
