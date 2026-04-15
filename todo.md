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
- removed object-local `sync_coordination_category`,
  `home_slot_category`, and `window_coordination_category` after confirming
  the live binding-prerequisite and handoff route already reads those facts
  from contention plus batch-owned summaries instead of from per-object
  mirrors
- dropped the now-dead category assignments in
  `populate_object_allocation_state(...)` so register-candidate, missing-state,
  and fixed-stack objects stop publishing coordination mirrors that had no
  non-test consumer
- updated the prepare-entry contract test to keep coordination-category
  assertions on contention summaries while removing the deleted object-local
  mirror checks

## Suggested Next
- inspect whether object-local `binding_batch_kind` and
  `binding_order_index` are still the minimal per-object attachment contract or
  whether any remaining per-object publication can shrink further without
  taking batch ownership away from batch summaries
- keep the next packet inside step 5 ownership cleanup; do not turn it into
  new allocation policy or target-ingestion work

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
- object-local coordination categories are gone now; keep future cleanup aimed
  at mirrors with real summary-level owners instead of recreating new
  fixed-stack or missing-state publication layers
- object-local batch attachment still looks intentional because tests and
  downstream reporting need to map each object onto its owning batch entry; do
  not delete that projection unless the next packet can prove another stable
  attachment path

## Proof
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed; the full `^backend_` subset completed successfully with output in
  `test_after.log`
