# BIR Semantic Global/Static GEP Admission Producer

Status: Closed
Type: Focused BIR semantic producer implementation idea
Parent: `ideas/closed/499_semantic_gep_local_memory_admission_producer.md`
Source Evidence:
- `build/agent_state/499_step1_gep_local_memory_classification/summary.md`
- `build/agent_state/499_step1_gep_local_memory_classification/classification_counts.tsv`
- `build/agent_state/499_step1_gep_local_memory_classification/representative_shapes.tsv`
- `build/agent_state/499_step3_gep_local_memory_producer/summary.md`
Owning Layer: BIR semantic global/static GEP admission producer

## Goal

Classify and repair the six global/static object rows separated from the
semantic `gep local-memory` family without inferring global object, layout,
range, or provenance authority in RV64/MIR lowering.

## Why This Exists

Idea 499 completed the direct local-object GEP producer from available
local-address provenance. Its Step 1 classification separated six
`global_or_static_object_gep_boundary` rows, including `src/20000717-4.c`,
`src/20031214-1.c`, `src/20051104-1.c`, `src/20080424-1.c`,
`src/20120808-1.c`, and `src/ieee/copysign2.c`.

Those rows are not local-object GEPs and cannot consume
`local_array_semantic_geps`. They need explicit global/static object identity,
layout/range, and provenance authority before semantic GEP admission or any
RV64 consumer can proceed.

## In Scope

- Reproduce the six global/static object GEP rows from idea 499 classification.
- Audit existing global/static object, selected data, layout, relocation, and
  provenance surfaces that could authorize semantic GEP admission.
- Define a narrow semantic global/static GEP admission contract if the current
  producer surfaces are sufficient.
- Preserve fail-closed statuses for missing global object identity, missing
  layout/range, missing provenance, relocation/selected-data gaps, raw-shape
  evidence, and target-only evidence.

## Out Of Scope

- RV64/MIR lowering that reconstructs global/static object authority from raw
  BIR/LIR shape, names, labels, final homes, object files, or target behavior.
- Direct local-object GEPs already owned by idea 499.
- Pointer/formal provenance, runtime/string intrinsics, aggregate/member or
  flexible-layout aliases, variadic routing, store local-memory, direct-call,
  scalar/local-memory, memcpy/memset, alloca, F128, move-bundle, runtime
  mismatch, stack-frame, or scalar-load work.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Acceptance Criteria

- The six global/static GEP rows are classified with representatives and
  first-owner decisions.
- Available semantic global/static GEP admission requires explicit global/static
  source object, layout/range, provenance, and coordinate authority.
- Missing object identity, missing layout/range, missing provenance,
  relocation/selected-data gaps, raw-shape-only, target-only, and coordinate
  confusion remain distinguishable and fail closed.
- Any RV64/MIR consumer is sequenced after producer facts close with proof.

## Reviewer Reject Signals

Reject this idea as progress if the change:

- admits global/static GEPs from testcase names, global labels, value names,
  dump order, final homes, object files, RV64 relocations, or target behavior;
- reconstructs global object layout, range, or provenance in RV64/MIR lowering
  instead of publishing BIR semantic authority;
- weakens unavailable statuses or treats raw shape as available authority;
- folds local-object GEPs, pointer/formal provenance, runtime/string,
  aggregate/member, variadic, store, call, F128, move-bundle, stack-frame, or
  broad global-data work into this focused packet;
- changes expectations, unsupported markers, allowlists, pass/fail accounting,
  runtime comparison, or baseline logs as proof of progress.

## Completion

Closed after Step 4 published the final BIR semantic global/static GEP
admission producer surface:

- Step 1 reproduced and classified the six global/static GEP rows split out by
  idea 499.
- Step 2 defined the contract and routed the missing lower prerequisite to
  `global_static_gep_authority`.
- Step 3 published production `GlobalStaticGepAuthorityRecord` records during
  global GEP lowering.
- Step 4 published final `GlobalStaticSemanticGepRecord` /
  `global_static_semantic_geps` records from matching available
  `GlobalStaticGepAuthorityRecord` records.

The direct global/static GEP producer scope is complete. Pointer/string,
runtime/string intrinsic, aggregate/member, RV64/MIR lowering, and object
emission consumers remain outside this focused idea and should resume only
through separately owned open ideas.
