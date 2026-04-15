# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc contract activation

# Current Packet

## Just Finished
- added an explicit semantic-BIR `regalloc` artifact that groups prepared
  liveness objects per function and classifies them into target-neutral
  downstream allocation contracts: `register_candidate` vs
  `fixed_stack_storage`
- wired the shared semantic-BIR prepare entry through `regalloc` after
  `liveness` so the phase order now publishes a concrete step-4 artifact
  instead of stopping at liveness-only contracts
- extended the prepare entry-contract test so one semantic function now checks
  the new regalloc artifact alongside the existing prepare route, invariants,
  stack-layout objects, liveness contracts, and bootstrap-LIR fallback phase
  order

## Suggested Next
- refine the semantic-BIR regalloc artifact beyond coarse allocation classes
  by publishing use/def anchors, live ranges, or interference-friendly
  grouping that later physical assignment can consume without inferring back
  from raw BIR text
- keep any new regalloc detail keyed off prepared liveness and stack-layout
  artifacts plus explicit semantic structure rather than slot-name families
  or target heuristics

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
- keep the regalloc artifact inspectable and target-neutral; avoid replacing
  concrete classification data with broader notes
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
