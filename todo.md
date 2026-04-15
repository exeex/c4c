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
- reviewer report `review/prepare_prealloc_route_review.md` concluded the code
  still matches the source idea, but `todo.md` and `plan.md` had drifted toward
  an unsupported public `prealloc` rename and needed lifecycle repair
- `plan.md` now points at the real implementation files under
  `src/backend/prealloc/` while explicitly preserving `prepare` as the public
  shared-route contract for this idea

## Suggested Next
- inspect whether object-local `binding_batch_kind` and
  `binding_order_index` are still the minimal per-object attachment contract or
  whether more summary-owned publication can shrink without removing the last
  stable object-to-batch attachment path
- keep the next packet inside step-5 ownership cleanup in
  `src/backend/prealloc/regalloc.cpp` and related tests; do not turn it into a
  public API rename, new allocation policy, MIR ingestion, or target-specific
  work
- choose a build plus narrow backend proof for the packet, and expect a broader
  backend checkpoint before accepting another route-shaping slice

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
- the current priority is structural clarity; if a cleanup only renames symbols
  but does not reduce architectural ambiguity, prefer the cleanup that removes
  an actual ownership seam or mirror first

## Proof
- next executor packet should choose `build -> narrow backend subset`
- before accepting another route-shaping slice, require a broader backend
  checkpoint than the narrow three-test subset used during the rename wave
