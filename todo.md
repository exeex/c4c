# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- added an explicit target-neutral `priority_bucket` to each semantic-BIR
  `regalloc` object so prepared value-storage contracts now distinguish
  single-point, multi-point, and call-spanning register candidates from
  explicit prepared access-window and call-crossing cues
- kept the bucket classification inside prepare-owned semantics by deriving it
  from existing value-storage vs address-exposed contracts and real BIR
  instruction ordering instead of target register rules or physical
  assignment
- extended the prepare entry-contract test so representative single-point,
  call-spanning, and non-value prepared objects assert the new bucket naming
  alongside the existing access summary

## Suggested Next
- build on the new bucket contract by deciding whether prepared regalloc needs
  additional object-local cues for multi-point value storage, such as whether
  the last touch is a write or read, before any physical assignment work
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

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_prepare_entry_contract$' 2>&1 | tee
  test_after.log`
- passed and wrote `test_after.log`
