# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-3 first stack-layout artifact activation

# Current Packet

## Just Finished
- extended semantic-BIR `stack_layout` so it now publishes aggregate
  `llvm.va_arg.aggregate` output storage as a prepared frame object when the
  route is carried by explicit BIR call metadata and sret-style ABI facts
- kept the new stack-layout producer grounded in intrinsic identity plus first
  argument ABI/storage metadata instead of lowered local-slot-name inference
- tightened the prepare entry-contract test around one semantic function that
  now exercises legalized local slots, byval-copy storage, byval/sret
  parameter routes, aggregate call-result storage, and aggregate va_arg output
  storage in the shared prepare pipeline

## Suggested Next
- keep extending stack-layout artifact construction into other explicit
  prepared frame-storage producers only when prepare can identify them from
  semantic ABI facts or dedicated route metadata rather than local-slot naming
- stay focused on prepared stack-layout contract construction for later
  liveness/regalloc consumers; do not slide into target ingestion, frame
  offset assignment, or target-specific stack rebuilding

## Watchouts
- do not let the current stack-layout packet drift into target ingestion work that
  belongs to `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push stack-layout ownership back into semantic lowering or target
  codegen
- additional prepared frame-storage producers should only land when prepare can
  identify them from explicit semantic ABI facts, intrinsic identity, or
  dedicated lowering metadata instead of prefix/name matching on lowered local
  slots
- keep the phase artifact inspectable and target-neutral; avoid replacing
  concrete data with broader notes
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
