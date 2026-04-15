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
- removed object-level `binding_batch_kind` from
  `PreparedRegallocObject` and moved object-to-batch attachment onto new
  function-level `binding_attachments`, so ready/deferred objects keep their
  frontier state locally while batch identity now lives in one prepare-owned
  projection
- updated the prepare-entry contract test to assert batch attachment through
  that function-level projection while keeping ready ordering in
  `binding_sequence`, so this packet reduces another real regalloc mirror
  without renaming the public `prepare` contract

## Suggested Next
- after the required broader backend checkpoint, inspect whether ready
  `binding_attachments` still duplicate enough of `binding_sequence` that the
  ready frontier can publish attachment through one function-level owner while
  leaving deferred attachment explicit
- keep the next packet inside step-5 ownership cleanup in
  `src/backend/prealloc/regalloc.cpp` and related tests; do not turn it into a
  public API rename, new allocation policy, MIR ingestion, or target-specific
  work
- keep using build plus a narrow backend proof for packet work, but require
  the broader backend checkpoint before accepting another route-shaping slice

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
- ready and deferred batch identity now lives in
  `PreparedRegallocFunction::binding_attachments`; if later cleanup trims that
  view, keep one explicit function-level owner for deferred attachment instead
  of pushing batch identity back onto `PreparedRegallocObject`
- the current priority is structural clarity; if a cleanup only renames symbols
  but does not reduce architectural ambiguity, prefer the cleanup that removes
  an actual ownership seam or mirror first

## Proof
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_entry_contract$'`
- passed; proof output recorded in `test_after.log`
- before accepting another route-shaping slice, require a broader backend
  checkpoint than this narrow single-test subset
