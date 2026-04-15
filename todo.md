# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-3 explicit stack-layout source-kind activation

# Current Packet

## Just Finished
- added dedicated `bir::LocalSlotStorageKind::LoweringScratch` metadata so
  lowering-created scratch storage can be identified structurally instead of
  by slot-name prefixes
- taught semantic-BIR `stack_layout` to publish those metadata-tagged slots as
  `lowering_scratch_slot` prepared frame objects before the generic local-slot
  bucket
- extended the prepare entry-contract test so one semantic function now
  exercises the lowering-scratch route alongside generic locals,
  address-taken locals, byval-copy storage, phi-materialized storage,
  byval/sret parameter routes, aggregate call-result storage, and aggregate
  va_arg output storage in the shared prepare pipeline

## Suggested Next
- extend explicit prepared source kinds to any remaining aggregate scratch or
  spill-like storage only when lowering or legalize can publish dedicated
  route metadata instead of forcing prepare to infer intent from slot names
- start defining which of the now-published stack-layout source kinds need
  distinct downstream contracts once liveness begins consuming prepared frame
  objects

## Watchouts
- do not let the current stack-layout packet drift into target ingestion work that
  belongs to `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push stack-layout ownership back into semantic lowering or target
  codegen
- additional prepared frame-storage producers should only land when prepare can
  identify them from explicit semantic ABI facts, intrinsic identity, or
  dedicated lowering metadata instead of prefix/name matching on lowered local
  slots
- keep `LoweringScratch` scoped to genuinely lowering-owned scratch storage;
  do not start tagging ordinary locals or aggregate leaf slots just to grow
  the prepared source-kind list
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
