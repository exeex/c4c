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
- moved prepared liveness to an information-only owner of function call points,
  object access shape, direct read/write counts, access windows, and
  call-crossing cues
- removed regalloc-local access summary rebuilding so regalloc now consumes
  `PreparedLivenessObject` instead of rescanning BIR
- deleted several cosmetic regalloc-only contract layers
  (`preferred_register_pool`, pressure/reload/materialization/eligibility
  hints, `assignment_readiness`, `eviction_friction_hint`)
- deleted the synthetic ready-only `stable_binding_pass*` surface and kept
  binding consumption anchored on binding batches, binding order, and handoff
  summaries
- deleted object-level deferred-only binding projection fields and kept
  deferred objects on the same uniform `binding_*` and `binding_handoff_*`
  contract already used by ready objects
- removed the remaining object-level batch prerequisite and handoff mirrors so
  `PreparedRegallocObject` now keeps only binding frontier, batch membership,
  and order while batch and handoff summaries own the prerequisite/handoff
  detail
- rebuilt binding handoff summaries from batch-owned data instead of reading
  object-local mirrored prerequisite and handoff fields
- shrank `PreparedRegallocBindingDecision` so binding-sequence entries now keep
  only source identity, stage, batch membership, and binding order while batch
  and handoff summaries remain the owners of ordering policy, follow-up, and
  prerequisite detail
- updated `backend_prepare_entry_contract` to assert the slimmer per-binding
  contract and keep policy/prerequisite ownership checks on batch and handoff
  summaries

## Suggested Next
- continue shrinking `src/backend/prepare/regalloc.cpp` by removing any
  remaining helper or sequence surface that duplicates batch or handoff
  summary facts without adding new allocation facts
- prefer one uniform object contract plus function-level summaries; avoid
  parallel ready-vs-deferred object surfaces that encode the same prerequisite
  facts twice
- keep `PreparedRegallocFunction` focused on artifacts a downstream allocator
  would actually consume: staged candidates, contention summaries, binding
  sequence, and frontier summaries

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

## Proof
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_entry_contract$' 2>&1 | tee test_after.log`
- passed; the delegated build plus focused `backend_prepare_entry_contract`
  proof completed successfully and preserved output at `test_after.log`
