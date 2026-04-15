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
- removed `binding_frontier_kind` from
  `PreparedRegallocBindingHandoffSummary`; ready versus deferred handoff
  identity now stays derivable from the owning ready/deferred batch family
  joined by `binding_batch_kind`
- simplified the handoff-summary contract helper so it derives summaries
  directly from ready or deferred batch owners without threading an extra
  frontier-kind selector through regalloc
- updated the prepare-entry contract test to assert handoff reason plus
  batch-identity joins instead of treating handoff summaries as the owner of a
  second ready/deferred frontier tag

## Suggested Next
- inspect whether `binding_handoff_summary` itself still needs to exist as a
  separate published view or whether downstream consumers can join ready and
  deferred batch owners directly once handoff reason publication stays clear
- keep the next packet inside step-5 ownership cleanup in
  `src/backend/prealloc/regalloc.cpp` and related tests; do not turn it into a
  public API rename, new allocation policy, MIR ingestion, or target-specific
  work
- keep using build plus the backend subset proof for packet work while this
  route-shaping cleanup stays inside shared prepare ownership work

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
  packet removes more handoff publication, keep batch kind as the consumer join
  key instead of rebuilding ready/deferred ownership mirrors in the handoff
  layer
- handoff summaries now publish only `binding_frontier_reason` plus
  `binding_batch_kind`; if a downstream consumer still needs ready versus
  deferred identity, derive it from the owning batch family instead of
  reintroducing a duplicated frontier tag on the handoff view
- deferred binding batch construction now takes `deferred_reason` directly from
  the object; if later cleanup removes or reshapes that object field, confirm
  deferred-batch ownership still stays clear instead of recreating a view
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
- deferred batch identity still lives in
  `PreparedRegallocFunction::binding_attachments`; if later cleanup trims that
  view, keep one explicit function-level owner for deferred attachment instead
  of pushing batch identity back onto `PreparedRegallocObject` or inventing a
  fake deferred order contract
- the current priority is structural clarity; if a cleanup only renames symbols
  but does not reduce architectural ambiguity, prefer the cleanup that removes
  an actual ownership seam or mirror first

## Proof
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- passed; proof output recorded in `test_after.log`
- broader backend checkpoint completed for this packet
