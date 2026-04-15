# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-3 first stack-layout artifact activation

# Current Packet

## Just Finished
- added explicit semantic-BIR stack-layout artifacts to the prepare contract
  and taught the shared semantic route to run `stack_layout` after
  legalize
- made `stack_layout` emit concrete prepared stack objects from legalized
  local-slot data so the phase now leaves real target-neutral artifacts
  instead of only a note
- tightened the prepare entry-contract test around one legalized local slot so
  it proves phase order plus stack-object provenance, type, and
  size/alignment metadata

## Suggested Next
- extend the semantic-BIR stack-layout artifact beyond local slots so it also
  captures the prepared frame objects created by byval and sret memory routes
  without inventing target-specific offsets yet
- keep the next packet on stack-layout contract construction that later
  liveness/regalloc can consume, not target ingestion or note-only scaffolding

## Watchouts
- do not let the current stack-layout packet drift into target ingestion work that
  belongs to `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push stack-layout ownership back into semantic lowering or target
  codegen
- the first semantic-BIR stack-layout slice only publishes legalized local-slot
  objects; follow-up work should add nearby prepared frame-object families
  rather than jumping straight to target-specific offsets
- keep the phase artifact inspectable and target-neutral; avoid replacing
  concrete data with broader notes
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_'`
- passed and wrote `test_after.log`
