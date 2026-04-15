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
- removed the last deferred-batch use of
  `regalloc_binding_frontier_reason_view(...)` so deferred binding batch
  construction now derives its frontier reason directly from the real owners:
  object-owned `deferred_reason` for access-window deferrals and
  contention-owned `follow_up_category` for coordination deferrals
- deleted the now-dead frontier-reason view helper instead of leaving a mirror
  path behind for future batch construction
- preserved the existing deferred-frontier split where access-window deferred
  batches carry `awaiting_access_window_observation` in `deferred_reason`
  while coordination-deferred batches keep
  `batched_single_point_coordination` owned by contention summaries

## Suggested Next
- inspect whether object-local `binding_frontier_reason` still has any
  non-reporting consumer or whether step 5 can shrink that remaining mirror
  without removing downstream output that still depends on the object-local
  projection
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
- the handoff path now derives `binding_frontier_reason` from batch-owned
  `follow_up_category` or deferred-batch `deferred_reason`; do not reintroduce
  the object-level mirror as the internal owner for that publication path
- deferred binding batch construction now takes `deferred_reason` directly from
  the object; if later cleanup removes or reshapes that object field, confirm
  deferred-batch ownership still stays clear instead of recreating a view
- the ready frontier still derives its access-window prerequisite state from
  stage contention while deferred frontiers read it from batch-owned state; if
  the next cleanup tries to unify that, keep one clear owner for each fact

## Proof
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed; the full `^backend_` subset completed successfully with output in
  `test_after.log`
