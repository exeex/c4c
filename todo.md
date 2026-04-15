# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-1 prepare route ownership activation

# Current Packet

## Just Finished
- made the prepare entry contract explicit by splitting prepare-owned
  entrypoints between the shared semantic-BIR route and the bootstrap LIR
  fallback route, then routing `backend.cpp` through those helpers instead of
  leaving ownership implicit
- added `backend_prepare_entry_contract` coverage to prove the current phase
  contract: semantic BIR enters `prepare` through the shared prepared-BIR
  route, while bootstrap LIR remains an explicit fallback path

## Suggested Next
- inspect whether any remaining `backend.cpp` or prepare-facing wording still
  blurs the shared semantic-BIR route with the bootstrap LIR fallback, and
  tighten that route contract before widening into step-2 legality ownership
- keep the next packet on route ownership evidence and prepare-owned contract
  wording, not legality internals or target-ingestion behavior

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
