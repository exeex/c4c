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
- replaced the remaining attachment-keyed raw deferred-batch summary lookup in
  `tests/backend/backend_prepare_entry_contract_test.cpp` with an
  attachment-keyed `RegallocDeferredBatchMatch` path so the contract test
  reuses one explicit summary-plus-resolution surface for both deferred-reason
  and attachment-keyed checks
- kept the packet test-side only: no prepare-owned regalloc data model
  changes, no backend routing changes, and no public `prepare` contract rename
- reran the delegated build plus backend subset proof for the packet and kept
  the result in `test_after.log`

## Suggested Next
- inspect whether the remaining hybrid
  `find_regalloc_binding_batch_kind(...)` helper in
  `tests/backend/backend_prepare_entry_contract_test.cpp` still deserves to
  join ready and deferred batch identity behind one string-returning lookup
  now that both paths reuse explicit match surfaces
- keep the next packet inside step-5 regalloc ownership cleanup in related
  tests or `src/backend/prealloc/regalloc.cpp`; do not widen into a public API
  rename, new allocation policy, MIR ingestion, or target-specific work
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
- deferred prerequisite state now lives on deferred batch summaries while
  ready prerequisite/handoff facts rejoin through contention; if later cleanup
  trims more handoff fields, keep handoff as a consumer keyed by surviving
  owners instead of rebuilding prerequisite ownership there
- deferred frontier metadata now lives on deferred batch summaries while ready
  family joins stay on `binding_sequence` plus contention; if later cleanup
  trims more handoff mirrors, keep one clear answer for whether downstream
  joins or frontier-level aggregate views own each remaining fact
- deferred prerequisite state now rejoins from batch-owned
  `deferred_reason` plus prerequisite categories; do not restore separate
  deferred state mirrors unless a real downstream consumer cannot make that
  join explicit
- deferred access-window prerequisite state now rejoins from
  `PreparedRegallocDeferredBindingBatchSummary::deferred_reason`; do not add
  back a separate access-window category mirror unless a real downstream
  consumer proves `deferred_reason` is no longer enough
- deferred ordering policy now rejoins directly from deferred
  `deferred_reason`; do not recreate a second policy mirror unless a real
  consumer proves that reason no longer identifies the waiting policy
- deferred home-slot and sync-handoff prerequisite state now rejoins through
  `attachments -> object` for access-window waits and
  `attachments -> allocation decision -> contention_summary` for coordination
  waits; do not recreate summary mirrors unless a real consumer cannot make
  those joins explicit
- deferred-batch helper selection now funnels through one owner-choice helper
  in both regalloc and the prepare-entry contract test; if later cleanup trims
  more helper branching, keep that route explicit instead of spreading new
  `deferred_reason` switches back across multiple helpers
- deferred batch resolution in the contract test now caches identity plus the
  access-window-owner choice on `RegallocDeferredBatchJoin`; if later cleanup
  trims more helpers, keep that join result as the consumer surface instead of
  recreating separate owner-object or owner-contention wrappers
- deferred batch helper queries for stage, ordering, and prerequisite/handoff
  state now share `RegallocDeferredBatchResolution`; if later cleanup trims
  lookup helpers too, keep one explicit resolved-batch surface instead of
  pushing new join calls back into each query helper
- deferred batch lookup now goes through one match helper surface for both
  deferred-reason and attachment-keyed checks; if later cleanup trims more
  helper branching, keep one stable consumer surface for identity-plus-
  resolution joins instead of splitting those fields back apart
- deferred batch identity checks in the contract test now funnel through one
  explicit identity helper; if a later packet trims more helper branching,
  keep that route instead of re-scattering new `deferred_reason` checks across
  lookups and assertions
- the handoff lookup path now keys tests by `binding_batch_kind`; if a later
  packet trims more binding mirrors, keep batch kind as the consumer join key
  instead of rebuilding ready/deferred ownership mirrors in a new handoff
  layer
- handoff reason ownership now lives on deferred batch summaries and ready
  contention summaries; do not recreate a separate aggregate handoff
  publication unless a real downstream consumer proves those owners are
  insufficient
- deferred binding batch construction now takes `deferred_reason` directly from
  the object; if later cleanup removes or reshapes that object field, confirm
  deferred-batch ownership still stays clear instead of recreating a view
- deferred batch handoff-family identity now rides on
  `binding_batch_kind` while the specific deferred blocker rides on
  `deferred_reason`; do not restore a second deferred summary mirror unless a
  real consumer cannot rejoin those surviving owners
- deferred batch family identity now rejoins through
  `attachments -> first object -> allocation decision / contention` plus
  `deferred_reason`; do not restore a stored summary `binding_batch_kind`
  unless a real consumer proves the join is no longer stable enough
- ready batch family identity now rides on `binding_batch_kind` while ready
  member sequencing stays in `binding_sequence`; do not restore a ready batch
  summary layer for prerequisite, handoff, stage, ordering, or membership
  mirrors unless a real consumer cannot rejoin those surviving owners
- ready prerequisite and handoff facts now rejoin through
  `allocation_stage -> contention_summary`; if later cleanup reshapes ready
  family identity, keep that join explicit instead of recreating a second ready
  publication view
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
- backend subset proof completed for this packet, including `backend_prepare_entry_contract`
