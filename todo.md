# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-3 explicit stack-layout source-kind activation

# Current Packet

## Just Finished
- extended semantic-BIR `stack_layout` so local slots reached through explicit
  `bir::MemoryAddress::LocalSlot` accesses are now published as
  `address_taken_local_slot` prepared frame objects instead of being folded
  into the generic local-slot bucket
- kept the new classifier grounded in structural BIR route metadata
  (`LoadLocalInst`/`StoreLocalInst` address operands, plus any future
  `is_address_taken` lowering metadata) rather than slot naming or testcase
  text matching
- tightened the prepare entry-contract test around one semantic function that
  now exercises generic local slots, address-taken local storage, byval-copy
  storage, phi-materialized storage, byval/sret parameter routes, aggregate
  call-result storage, and aggregate va_arg output storage in the shared
  prepare pipeline

## Suggested Next
- keep extending stack-layout artifact construction into other explicit
  prepared frame-storage producers only when prepare can identify them from
  semantic ABI facts or dedicated route metadata rather than local-slot naming
- decide whether aggregate scratch storage introduced by legalize or memory
  lowering needs its own prepared source kinds before liveness/regalloc start
  consuming stack-layout artifacts

## Watchouts
- do not let the current stack-layout packet drift into target ingestion work that
  belongs to `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push stack-layout ownership back into semantic lowering or target
  codegen
- additional prepared frame-storage producers should only land when prepare can
  identify them from explicit semantic ABI facts, intrinsic identity, or
  dedicated lowering metadata instead of prefix/name matching on lowered local
  slots
- keep phi-materialize classification tied to `phi_observation`; do not infer
  it from slot names or printed BIR text
- keep address-taken classification tied to structured BIR address operands or
  dedicated lowering metadata; do not infer it from slot names
- keep the phase artifact inspectable and target-neutral; avoid replacing
  concrete data with broader notes
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
