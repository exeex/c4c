# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR liveness contract activation

# Current Packet

## Just Finished
- added an explicit semantic-BIR `liveness` artifact that consumes prepared
  stack-layout objects and classifies them into target-neutral downstream
  contracts: `value_storage` vs `address_exposed_storage`
- wired the shared semantic-BIR prepare entry through `liveness` after
  `stack_layout` so the phase order now publishes a concrete step-4 artifact
  instead of stopping at stack-layout-only notes
- extended the prepare entry-contract test so one semantic function now checks
  the new liveness artifact alongside the existing prepare route, invariants,
  stack-layout objects, and bootstrap-LIR fallback phase order

## Suggested Next
- refine semantic-BIR liveness beyond contract classification by publishing
  per-function live object groupings or use/def anchors that downstream
  regalloc can consume without inferring back from raw BIR text
- keep any new liveness detail keyed off prepared stack-layout artifacts and
  explicit semantic structure rather than slot-name families or target
  heuristics

## Watchouts
- do not let the current liveness packet drift into target ingestion work that
  belongs to `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push prepare ownership back into semantic lowering or target codegen
- keep downstream liveness contracts driven by prepared stack-layout
  `source_kind` ownership rather than target-specific rules or slot-name
  pattern matching
- if more source kinds appear, classify them from explicit semantic ABI facts,
  intrinsic identity, or lowering metadata instead of inventing name-based
  buckets inside liveness
- keep the liveness artifact inspectable and target-neutral; avoid replacing
  concrete classification data with broader notes
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
