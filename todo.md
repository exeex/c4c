# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-1 prepare route ownership activation

# Current Packet

## Just Finished
- made the prepare entry contract explicit on prepared-module surfaces with a
  concrete `PrepareRoute`, then funneled `backend.cpp` through named semantic
  BIR and bootstrap LIR prepare helpers instead of leaving that route split as
  ad hoc calls
- tightened `backend_prepare_entry_contract` so the current slice proves the
  route contract through explicit prepare-owned route identifiers as well as
  the existing route notes and phase-order expectations

## Suggested Next
- start step 2 by inventorying which target-facing semantic-BIR forms still
  rely on target-side assumptions today, then sketch the first prepare-owned
  legality invariant in `legalize.cpp` and `prepare.cpp` without reopening raw
  route growth
- keep the next packet on general legality ownership, not target-ingestion
  work or testcase-shaped target shortcuts

## Watchouts
- do not let the first packet drift into target ingestion work that belongs to
  `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push legality or prepared-phase ownership back into semantic lowering
- the new entry contract is intentionally honest about the current bootstrap
  LIR fallback; do not “solve” that by reopening raw-route growth instead of
  continuing the prepare-owned semantic-BIR route
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_'`
- passed and wrote `test_after.log`
