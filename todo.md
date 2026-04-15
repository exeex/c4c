# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-3 first stack-layout artifact activation

# Current Packet

## Just Finished
- extended semantic-BIR `stack_layout` so phi-observation-backed local slots
  are now published as explicit `phi_materialize_slot` prepared frame objects
  instead of being folded into the generic local-slot bucket
- kept the new classifier grounded in dedicated lowering metadata
  (`phi_observation`) rather than local-slot naming or testcase-shaped
  heuristics
- tightened the prepare entry-contract test around one semantic function that
  now exercises generic local slots, byval-copy storage, phi-materialized
  storage, byval/sret parameter routes, aggregate call-result storage, and
  aggregate va_arg output storage in the shared prepare pipeline

## Suggested Next
- keep extending stack-layout artifact construction into other explicit
  prepared frame-storage producers only when prepare can identify them from
  semantic ABI facts or dedicated route metadata rather than local-slot naming
- consider whether address-taken local storage or other legality-emitted frame
  objects need their own prepared source kinds before liveness/regalloc start
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
- keep the phase artifact inspectable and target-neutral; avoid replacing
  concrete data with broader notes
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed and wrote `test_after.log`
