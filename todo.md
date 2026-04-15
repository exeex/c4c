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
- stopped binding handoff summary construction from reading a mirrored
  object-level `binding_frontier_reason` fallback when the ready path already
  owns that fact in batch `follow_up_category` and the deferred path already
  owns it in deferred-batch `deferred_reason`
- kept object-local `binding_frontier_reason` publication intact for downstream
  consumers while shrinking handoff publication back toward
  `binding_batches`/`deferred_binding_batches`
- preserved the existing deferred-frontier split where access-window deferred
  handoffs publish `awaiting_access_window_observation` and coordination
  deferred handoffs publish the contention-owned follow-up category

## Suggested Next
- keep shrinking step-5 contract mirrors by checking whether deferred-batch
  construction still needs `regalloc_binding_frontier_reason_view(...)` or can
  publish its `deferred_reason` directly from `deferred_reason` plus
  contention-owned follow-up state without changing object-local frontier
  output
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
- the ready frontier still derives its access-window prerequisite state from
  stage contention while deferred frontiers read it from batch-owned state; if
  the next cleanup tries to unify that, keep one clear owner for each fact

## Proof
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed; the full `^backend_` subset completed successfully with output in
  `test_after.log`
