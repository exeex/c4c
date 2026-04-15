# Prealloc Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: finish the `prepare -> prealloc` structural cleanup so the backend pipeline reads cleanly as `bir -> prealloc -> mir`

# Current Packet

## Goal
- finish turning `src/backend/prealloc/` into a coherent class-owned API surface
  and naming domain so the shared backend flow is clearly `bir -> prealloc -> mir`
- keep prealloc liveness as the sole owner of access facts and keep regalloc as
  a consumer that stages reservation, contention, binding, and handoff without
  rebuilding liveness or inventing extra contract layers

## Just Finished
- renamed the prealloc entry surface to `prealloc.hpp/.cpp`, deleted the
  phase-local headers, and made `BirPreAlloc` the only public API index
- removed the old bootstrap-LIR prealloc fallback route so the main public
  surface is BIR-only
- simplified `src/backend/backend.cpp` down to the shared pipeline orchestration
  path instead of keeping target-specific RISC-V lowering logic in the driver
- changed `BirPreAlloc` phase execution from static namespace-like helpers into
  instance methods that operate on owned `prepared_` state
- moved the regalloc function-local helper flow onto `BirPreAlloc` state so the
  regalloc packet no longer threads `PreparedRegallocFunction&` through every
  helper
- aligned top-level entry notes and regalloc prerequisite/handoff state strings
  from `prepare_*` naming to `prealloc_*`

## Suggested Next
- rename the remaining `prepare` API and namespace surface to `prealloc`:
  `namespace c4c::backend::prepare`, `PrepareRoute`,
  `prepare_route_name(...)`, `prepare_semantic_bir_module_with_options(...)`,
  and `prepare_bir_module_with_options(...)`
- update backend/tests to consume the renamed `prealloc` API surface without
  changing pipeline behavior
- after the API rename, inspect whether object-local `binding_batch_kind` and
  `binding_order_index` are still the minimal per-object attachment contract or
  whether any remaining per-object publication can shrink further without
  taking batch ownership away from batch summaries
- keep the next packets inside ownership and naming cleanup; do not turn them
  into new allocation policy, MIR ingestion, or target-specific work

## Watchouts
- do not add more liveness-like fact gathering to
  `src/backend/prealloc/regalloc.cpp`
- do not leave the codebase in a half-renamed state where directory/class names
  say `prealloc` but namespace/API names still advertise `prepare`
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
  an actual ownership seam or namespace mismatch first

## Proof
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_phi_materialize|backend_prepare_entry_contract|backend_lir_to_bir_notes)$'`
- passed; the focused backend prealloc subset completed successfully after the
  latest `BirPreAlloc` ownership and contract-naming cleanup
