# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: continue prepare-route cleanup in `src/backend/prealloc/`
while keeping the public shared contract named `prepare`

# Current Packet

## Goal
- keep regalloc cleanup inside the active `prepare` idea by reducing real
  ownership ambiguity in `src/backend/prealloc/` rather than renaming the
  public `prepare` namespace/API surface
- keep liveness as the sole owner of access facts and keep regalloc as a
  consumer that stages reservation, contention, binding, and handoff without
  rebuilding liveness or inventing extra contract layers

## Just Finished
- removed ready binding batch `ordering_policy` because ready frontier
  ordering already stays clear through `binding_sequence` plus
  `binding_batch_kind`
- kept deferred batch `ordering_policy` intact because deferred frontier
  batches still own distinct wait semantics that are not recoverable from the
  ready sequencing join
- refreshed the prepare-entry contract test so ready batch expectations now
  prove ordering through `binding_sequence` and preserve batch summaries only
  for prerequisite and candidate aggregation

## Suggested Next
- inspect whether ready batch `candidate_count` still needs to live on the
  summary once ready membership and ordering already rejoin through
  `binding_sequence` plus `binding_batch_kind`
- keep the next packet inside step-5 ownership cleanup in
  `src/backend/prealloc/regalloc.cpp` and related tests; do not turn it into a
  public API rename, new allocation policy, MIR ingestion, or target-specific
  work
- keep using build plus the backend subset proof while this cleanup stays
  inside shared prepare ownership work

## Watchouts
- do not add more liveness-like fact gathering to
  `src/backend/prealloc/regalloc.cpp`
- do not rename `namespace c4c::backend::prepare`, `PrepareRoute`, or the
  `prepare_*` public entrypoints to `prealloc` under this idea unless a later
  lifecycle checkpoint explicitly changes the runbook
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
- ready/deferred prerequisite state now lives on batch summaries; if later
  cleanup trims more handoff fields, keep handoff as a consumer keyed by
  `binding_batch_kind` instead of rebuilding prerequisite ownership there
- batch metadata now lives on ready/deferred batch summaries too; if later
  cleanup trims more handoff mirrors, keep one clear answer for whether
  downstream joins or frontier-level aggregate views own each remaining fact
- the handoff lookup path now keys tests by `binding_batch_kind`; if a later
  packet trims more binding mirrors, keep batch kind as the consumer join key
  instead of rebuilding ready/deferred ownership mirrors in a new handoff
  layer
- handoff reason ownership now lives directly on ready/deferred batch
  summaries; do not recreate a separate aggregate handoff publication unless a
  real downstream consumer proves batch-owned fields are insufficient
- deferred binding batch construction now takes `deferred_reason` directly from
  the object; if later cleanup removes or reshapes that object field, confirm
  deferred-batch ownership still stays clear instead of recreating a view
- deferred batch handoff-family identity now rides on
  `binding_batch_kind` while the specific deferred blocker rides on
  `deferred_reason`; do not restore a second deferred summary mirror unless a
  real consumer cannot rejoin those surviving owners
- ready batch family identity now rides on `binding_batch_kind` while ready
  member sequencing stays in `binding_sequence`; do not restore ready summary
  `allocation_stage`, `follow_up_category`, or `ordering_policy` mirrors
  unless a real consumer cannot rejoin those surviving owners
- ready access-window prerequisite state now lands on `binding_batches` from
  stage contention while deferred frontier prerequisites stay on deferred batch
  summaries; if later cleanup tries to unify derivation, keep one clear owner
  for each fact
- object-local coordination categories are gone now; keep future cleanup aimed
  at mirrors with real summary-level owners instead of recreating new
  fixed-stack or missing-state publication layers
- ready batch identity now lives in `PreparedRegallocFunction::binding_sequence`;
  do not re-expand `binding_attachments` back into the ready frontier unless a
  downstream consumer proves sequence ownership is insufficient
- deferred batch identity now lives on
  `PreparedRegallocDeferredBindingBatchSummary::attachments`; if later cleanup
  trims that view, keep deferred membership owned by batch summaries instead of
  pushing batch identity back onto `PreparedRegallocObject` or recreating a
  parallel function-level attachment index
- deferred attachment membership now rejoins through
  `PreparedRegallocFunction::objects` by stable object index; if later cleanup
  reshapes regalloc object ordering, keep one clear stable join for deferred
  frontier identity instead of restoring duplicated source-name mirrors
- deferred batch membership count now comes from `attachments.size()`; do not
  reintroduce a separate deferred `candidate_count` field unless a real
  consumer needs a count that differs from attachment membership
- deferred batch stage identity now rejoins through attachment-owned objects
  back into `allocation_sequence`; if later cleanup changes that join, keep one
  stable owner for deferred stage identity instead of restoring a summary-level
  `allocation_stage` mirror
- the current priority is structural clarity; if a cleanup only renames symbols
  but does not reduce architectural ambiguity, prefer the cleanup that removes
  an actual ownership seam or mirror first

## Proof
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- passed; proof output recorded in `test_after.log`
- backend subset proof completed for this packet
