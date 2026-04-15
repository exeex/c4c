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
- removed object-local `PreparedRegallocObject::follow_up_category` after
  confirming the downstream binding and handoff route already reads
  follow-up ownership from contention, binding-batch, and handoff summaries
- dropped the now-dead object-level assignments in
  `populate_object_allocation_state(...)` so register-candidate and
  fixed-stack objects no longer mirror contention follow-up classification
- updated the prepare-entry contract test to stop asserting the deleted
  object-local mirror while keeping follow-up checks on contention,
  binding-batch, and handoff summaries where that contract still lives

## Suggested Next
- inspect whether object-local `sync_coordination_category`,
  `home_slot_category`, and `window_coordination_category` are likewise pure
  mirrors of contention or fixed-stack ownership and remove only the fields
  that have no surviving non-test consumer
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
- the remaining object-local coordination categories appear to be the next
  mirrored layer, but fixed-stack objects still synthesize their values
  locally; confirm whether those fixed-stack projections are consumed anywhere
  real before deleting them

## Proof
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- passed; the full `^backend_` subset completed successfully with output in
  `test_after.log`
