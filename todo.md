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
- changed regalloc handoff publication to consume
  `binding_batches`/`deferred_binding_batches` directly, so
  `populate_binding_handoff_summary()` no longer rescans per-object
  `binding_batch_kind` mirrors to rebuild a batch-owned contract
- kept the object-local `binding_batch_kind` / `binding_order_index` attachment
  path intact for downstream mapping and reporting, so this packet shrank the
  handoff ownership seam without deleting the last stable object-to-batch link

## Suggested Next
- inspect whether object-local `binding_batch_kind` and
  `binding_order_index` are still the minimal per-object attachment contract or
  whether one of them can shrink further now that handoff publication is fully
  batch-owned, without removing the last stable object-to-batch attachment path
- keep the next packet inside step-5 ownership cleanup in
  `src/backend/prealloc/regalloc.cpp` and related tests; do not turn it into a
  public API rename, new allocation policy, MIR ingestion, or target-specific
  work
- keep using build plus a narrow backend proof for packet work, but expect a
  broader backend checkpoint before accepting another route-shaping slice

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
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_entry_contract$'`
- passed; proof output recorded in `test_after.log`
- before accepting another route-shaping slice, require a broader backend
  checkpoint than this narrow single-test subset
