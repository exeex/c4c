# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-4 semantic-BIR regalloc bucket activation

# Current Packet

## Just Finished
- added one representative non-call-spanning `window.slot` value-storage case
  to the prepare entry-contract fixture with a real direct store followed by a
  later direct load after the existing calls, so the regalloc contract now
  proves a genuine `multi_point_read_write_candidate` path from actual BIR
  instruction order
- extended the same fixture’s stack-layout, liveness, register-candidate count,
  and regalloc assertions so the new slot is checked end-to-end as prepared
  value storage rather than a synthetic interval or target-shaped heuristic

## Suggested Next
- extend the prepare entry-contract fixture with one representative multi-point
  read-only or write-only value-storage case so the remaining non-call-spanning
  readiness shapes are still proved from actual BIR access order instead of
  synthetic intervals or target register policy

## Watchouts
- do not let the current regalloc packet drift into target ingestion work that
  belongs to `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push prepare ownership back into semantic lowering or target codegen
- keep downstream regalloc contracts driven by prepared liveness and
  stack-layout ownership rather than target-specific rules or slot-name pattern
  matching
- if more source kinds appear, classify them from explicit semantic ABI facts,
  intrinsic identity, or lowering metadata instead of inventing name-based
  buckets inside liveness/regalloc
- keep the regalloc artifact inspectable and target-neutral; prefer concrete
  access summaries over broad notes, but do not fake live ranges or
  interference until the data is really available
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available
- if follow-on readiness cues are added, keep them sourced from actual BIR
  access order, liveness contracts, and call-crossing facts rather than
  synthetic interval guesses or phi-materialization scaffolding that does not
  exercise a real local-slot access window

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_prepare_entry_contract$' 2>&1 | tee
  test_after.log`
- passed and wrote `test_after.log`
