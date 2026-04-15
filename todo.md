# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-1 prepare route ownership activation

# Current Packet

## Just Finished
- activated `ideas/open/48_prepare_pipeline_rebuild.md` into `plan.md`

## Suggested Next
- inspect `src/backend/prepare/prepare.cpp` and `src/backend/backend.cpp` to
  name the current prepare entry contract and the exact shell behavior that
  still prevents `prepare` from owning the shared route
- keep the first executor packet bounded to route ownership and contract
  discovery under step 1 before widening into legality or phase rebuild work

## Watchouts
- do not let the first packet drift into target ingestion work that belongs to
  `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push legality or prepared-phase ownership back into semantic lowering
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available

## Proof
- activation only; no code proof run yet
