# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-5 regalloc consumer shrink

# Current Packet

## Goal
- keep prepare liveness as the sole owner of access facts and keep regalloc as
  a consumer that stages reservation, contention, binding, and handoff without
  rebuilding liveness or inventing extra contract layers

## Just Finished
- replaced the duplicated ready-vs-deferred `binding_handoff_summary`
  construction path with one shared batch-contract view that materializes
  handoff summaries from batch-owned data regardless of frontier kind
- kept ready and deferred batch summary ownership unchanged while removing the
  separate handoff push paths that previously rebuilt the same contract fields
  in parallel
- preserved the focused `backend_prepare_entry_contract` proof while shrinking
  the regalloc handoff seam to one contract-to-summary conversion path

## Suggested Next
- continue shrinking `src/backend/prepare/regalloc.cpp` by removing any
  remaining ready-vs-deferred partition bookkeeping that now only mirrors
  `binding_batches` and `binding_handoff_summary` counts instead of adding new
  allocation facts
- treat the next packet as a function-summary cleanup, not a new analysis
  feature: if a count or batch field can be derived from the existing shared
  batch frontier surface, prefer deleting the mirror

## Watchouts
- do not add more liveness-like fact gathering to
  `src/backend/prepare/regalloc.cpp`
- do not drift into target ingestion or target-specific register policy
- do not re-introduce synthetic pass layers, fake intervals, placeholder
  interference facts, or name-based special cases just to flatten the contract
- preserve the split between register-candidate reservation decisions and
  fixed-stack authoritative objects
- if a regalloc field only repeats a function-level batch/handoff summary onto
  every object, prefer deleting it unless a downstream consumer truly needs the
  object-local projection
- the ready frontier still derives its access-window prerequisite state from
  stage contention while deferred frontiers read it from batch-owned state; if
  the next cleanup tries to unify that too, keep one clear owner for each fact

## Proof
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_entry_contract$' 2>&1 | tee test_after.log`
- passed; the build plus focused `backend_prepare_entry_contract` proof
  completed successfully and preserved output at `test_after.log`
