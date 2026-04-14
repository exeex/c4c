# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md

## Activation Decision

- 2026-04-14 activation result:
  activate `ideas/open/47_semantic_call_runtime_boundary.md` as the next
  executable backend follow-on
- rationale:
  this is the immediate continuation of the shared semantic-lowering route
  after the closed backend-reboot idea; `ideas/open/48_prepare_pipeline_rebuild.md`
  and `ideas/open/49_prepared_bir_target_ingestion.md` both depend on semantic
  BIR staying honest first, and idea 49 explicitly depends on both earlier
  tracks
- route constraint:
  keep this runbook focused on semantic call/runtime boundary work and do not
  silently pull `prepare` reconstruction or prepared-BIR target-ingestion work
  forward

## Current Active Item

- active route:
  semantic BIR remains the truth surface, `prepare` remains the legality
  owner, and target reconnection stays deferred to later follow-on ideas
- current capability family:
  semantic call ownership and runtime-facing lowering
- current packet focus:
  semantic runtime/intrinsic lowering now covers integer `abs` plus direct
  `memcpy` lowering for matching local aggregates and nested GEP-derived local
  subobjects in `lir_to_bir`; the next packet should stay on adjacent
  runtime/call families instead of drifting into `prepare`
- packet rule:
  the first executor packet must change shared semantic lowering, not just add
  new test observers or rename unsupported cases
- proof expectation:
  choose a proving subset from backend-route or internal backend cases that
  exercises one semantic call/runtime family with a fresh build before broader
  validation
- Just Finished:
  lowered direct `@memcpy(ptr, ptr, i64)` calls into semantic BIR for matching
  local aggregate objects and matching GEP-derived local aggregate subobjects,
  taught aggregate leaf helpers to respect subobject-relative offsets, aliased
  the pointer return to the destination object, and added backend-route proof
  with `builtin_memcpy_nested_pair_field.c`
- Watchouts:
  direct `memcpy` calls whose operands arrive as local arrays, mixed-shape
  pointers, or non-local bases still fall back today; keep follow-on work in
  shared semantic lowering rather than target- or testcase-shaped handling
- Proof:
  `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
  passed, including
  `backend_codegen_route_x86_64_builtin_memcpy_local_pair_observe_semantic_bir`
  and
  `backend_codegen_route_x86_64_builtin_memcpy_nested_pair_field_observe_semantic_bir`;
  `test_after.log` preserved

## Suggested Next

- extend semantic `memcpy` lowering from matching local aggregate objects to
  the next nearby shared shape that still stays semantic and planner-honest,
  preferably local array objects or another shared local-subobject path
- if that next `memcpy` shape needs new semantic BIR surface, keep that
  surface shared and planner-honest rather than encoding direct target
  behavior
