# Semantic LIR-To-BIR Admission High-Impact Cleanup

Status: Closed
Type: BIR semantic producer admission cleanup idea
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`
Owning Layer: BIR semantic producer, before RV64 prepared-object consumption
Resumed After: ideas/closed/483_bir_semantic_local_memory_scalar_load_producer.md
Closed By: lifecycle review after Step 2 next-owner selection

## Goal

Classify and repair high-frequency `semantic lir_to_bir` admission failures
from the current RV64 gcc_torture backend object path without moving producer
responsibility into MIR/RV64 lowering.

## Why This Exists

The 2026-07-01 supervisor scan recorded `373` failures whose first owner is
semantic LIR-to-BIR admission. Representative logs show local-memory load/store
families, scalar/local-memory families, function-signature cases, global
admission limits, and runtime/intrinsic families such as `memcpy`/`memset`.

These are producer gaps. RV64 should consume prepared semantic facts only after
the BIR producer admits and publishes them truthfully.

Idea 483 now publishes `local_array_scalar_local_loads` for clean local-array
scalar local loads. This cleanup should reclassify the semantic-admission
bucket using that surface and route only remaining producer gaps; RV64 lowering
must consume the fact surface rather than reconstructing local-load authority.

## Completion Notes

Idea 496 is complete as the post-scalar-local-load semantic admission routing
container.

Completed evidence:

- `build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/summary.md`
- `build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/semantic_family_counts.tsv`
- `build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/semantic_family_representatives.tsv`
- `build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/local_load_disposition.tsv`
- `build/agent_state/496_step2_next_owner_selection/decision.md`
- `build/agent_state/496_step2_next_owner_selection/open_idea_inventory.tsv`

Step 2 selected the focused BIR semantic `gep local-memory` admission producer
as the next owner for the `62`-row family represented by `src/pr44468.c`,
`src/pr48571-1.c`, `src/pr65956.c`, `src/pr80421.c`, and
`src/20000717-4.c`. No existing open idea was a clean, unblocked match, so the
next lifecycle state is:

`ideas/open/499_semantic_gep_local_memory_admission_producer.md`

The selected route is a producer packet, not an RV64/MIR consumer of
`local_array_scalar_local_loads`.

## Final Residual Disposition

Resolved local-array scalar-load rows should consume
`local_array_scalar_local_loads`. Remaining producer families stay split by
first owner: `gep local-memory` first, then store local-memory, direct-call,
scalar/local-memory, runtime/intrinsic, alloca, control-flow, signature, and
other smaller semantic families as later work.

## In Scope

- Reproduce the current semantic-admission bucket from
  `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`.
- Classify failures by semantic family: function signature, scalar/local
  memory, load/store local memory, global memory, calls, runtime/intrinsic
  helpers, and aggregate/global admission limits.
- Identify the smallest producer-owned follow-up ideas that unlock broad
  ordinary-C coverage.
- Define fail-closed diagnostics for families that remain outside current
  producer authority.
- Hand RV64 work forward only after producer facts close with proof.

## Out Of Scope

- RV64/MIR fixes that infer missing semantic facts.
- Instruction-fragment lowering for already-admitted BIR.
- Move-bundle materialization, except when a semantic producer gap must be
  routed to the move-bundle review.
- F128/long-double soft-float implementation.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Acceptance Criteria

- The handoff directory records semantic-admission counts, representative rows,
  and first-owner families.
- Each high-impact family has either a producer implementation follow-up, a
  precise blocked reason, or a documented lower prerequisite.
- Any MIR/RV64 idea that needs these facts is explicitly sequenced after the
  producer follow-up closes.
- Default `ctest --test-dir build -j --output-on-failure` must not regress for
  lifecycle close.

## Reviewer Reject Signals

- Reject RV64 lowering that reconstructs local-memory, global-memory,
  signature, call, or runtime-helper semantics from raw LIR/BIR shape.
- Reject mixing BIR producer repair and RV64 consumer lowering in one
  implementation slice.
- Reject testcase-shaped admission, named-case shortcuts, or diagnostics-only
  changes claimed as semantic producer progress.
