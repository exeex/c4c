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
- removed the remaining mirrored ready/deferred frontier counters from
  `PreparedRegallocFunction` now that binding totals are owned by
  `binding_sequence`, `binding_batches`, `deferred_binding_batches`, and
  `binding_handoff_summary`
- switched regalloc handoff reservation to the surviving batch vectors instead
  of batch-count mirrors and kept object-local frontier classification intact
- restated the focused `backend_prepare_entry_contract` checks against batch-
  and handoff-owned candidate counts rather than function-level duplicate
  counters

## Suggested Next
- continue shrinking `src/backend/prepare/regalloc.cpp` by checking whether
  any remaining per-object `binding_frontier_*` projections are still only
  re-expressing batch-owned prerequisites rather than adding allocation-state
  facts a downstream allocator would need
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
- the ready frontier still derives its access-window prerequisite state from
  stage contention while deferred frontiers read it from batch-owned state; if
  the next cleanup tries to unify that, keep one clear owner for each fact

## Proof
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_entry_contract$' 2>&1 | tee test_after.log`
- passed; the build plus focused `backend_prepare_entry_contract` proof
  completed successfully and preserved output at `test_after.log`
