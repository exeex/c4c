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
- stopped regalloc batch/handoff construction from reading per-object
  `binding_frontier_reason` mirrors when the same information already lives in
  object allocation-state fields and function-level batch summaries
- switched deferred batch prerequisite selection to `deferred_reason` for the
  access-window path and to contention-owned follow-up categories for the
  coordination-deferred path, keeping object-local frontier publication intact
- kept object-level `binding_frontier_*` output as downstream-facing state
  while shrinking internal summary ownership back toward
  `binding_batches`/`deferred_binding_batches`/`binding_handoff_summary`

## Suggested Next
- continue shrinking `src/backend/prepare/regalloc.cpp` by checking whether
  `binding_frontier_kind` itself can stop driving batch/handoff assembly for
  register-candidate objects, with publication preserved only where downstream
  consumers still need the object-local state
- keep the next packet focused on ownership cleanup inside step 5; do not turn
  it into new allocation policy or target-ingestion work

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
  the next cleanup tries to unify that, keep one clear owner for each fact

## Proof
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_entry_contract$' 2>&1 | tee test_after.log`
- passed; the build plus focused `backend_prepare_entry_contract` proof
  completed successfully
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- passed under `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
