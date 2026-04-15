# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-3 first stack-layout artifact activation

# Current Packet

## Just Finished
- extended semantic-BIR `stack_layout` so it now publishes prepared frame
  objects for explicit byval and sret parameter memory routes in addition to
  legalized local-slot artifacts
- split byval-copy local slots into their own prepared stack-object family so
  the stack-layout contract distinguishes copied aggregate storage from generic
  local-slot bookkeeping without inventing target offsets
- tightened the prepare entry-contract test around one semantic function that
  exercises legalized local slots, byval-copy storage, and byval/sret
  parameter routes in the shared prepare pipeline

## Suggested Next
- extend stack-layout artifact construction from parameter and byval-copy
  families into other explicit prepared frame-storage producers that are
  already modeled semantically, especially aggregate call-result storage, but
  only if the route can be derived from ABI metadata rather than slot-name
  heuristics
- keep the packet focused on stack-layout contract construction for later
  liveness/regalloc consumers; do not slide into target ingestion or offset
  assignment

## Watchouts
- do not let the current stack-layout packet drift into target ingestion work that
  belongs to `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push stack-layout ownership back into semantic lowering or target
  codegen
- aggregate call-result storage exists nearby, but the next slice should only
  claim it if prepare can identify the route from explicit semantic ABI facts
  instead of prefix/name matching on lowered local slots
- keep the phase artifact inspectable and target-neutral; avoid replacing
  concrete data with broader notes
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
