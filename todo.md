# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-3 first stack-layout artifact activation

# Current Packet

## Just Finished
- extended semantic-BIR `stack_layout` so it now publishes aggregate call-result
  sret storage as a prepared frame object when the route is carried by explicit
  semantic call ABI metadata instead of local-slot naming heuristics
- added explicit semantic BIR call metadata for sret-backed local result
  storage so prepare-owned stack-layout work can distinguish local aggregate
  call-result storage from forwarded sret parameters
- tightened the prepare entry-contract test around one semantic function that
  now exercises legalized local slots, byval-copy storage, byval/sret
  parameter routes, and aggregate call-result storage in the shared prepare
  pipeline

## Suggested Next
- extend stack-layout artifact construction from aggregate call-result storage
  into the next explicit prepared frame-storage producers that are already
  modeled semantically, but keep the route grounded in explicit ABI or prepare
  metadata instead of local-slot-name inference
- keep the packet focused on stack-layout contract construction for later
  liveness/regalloc consumers; do not slide into target ingestion, offset
  assignment, or target-specific stack rebuilding

## Watchouts
- do not let the current stack-layout packet drift into target ingestion work that
  belongs to `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push stack-layout ownership back into semantic lowering or target
  codegen
- additional prepared frame-storage producers should only land when prepare can
  identify them from explicit semantic ABI facts or dedicated lowering metadata
  instead of prefix/name matching on lowered local slots
- keep the phase artifact inspectable and target-neutral; avoid replacing
  concrete data with broader notes
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
