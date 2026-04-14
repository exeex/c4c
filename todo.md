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
  semantic runtime/intrinsic lowering now covers integer `abs`, direct
  `memcpy` lowering for matching local aggregates and nested GEP-derived local
  subobjects plus matching local `i32` arrays, and direct zero-fill `memset`
  calls for the same local aggregate/local array families in `lir_to_bir`;
  the next packet should stay on adjacent runtime/call families instead of
  drifting into `prepare`
- packet rule:
  the first executor packet must change shared semantic lowering, not just add
  new test observers or rename unsupported cases
- proof expectation:
  choose a proving subset from backend-route or internal backend cases that
  exercises one semantic call/runtime family with a fresh build before broader
  validation
- Just Finished:
  lowered direct `@memset(ptr, int, i64)` zero-fill calls into semantic BIR
  for matching local aggregate objects and local array views, reusing the
  shared local zeroing route already used by semantic `memset` ops, aliasing
  the pointer result back to the destination, and adding backend-route proof
  with `builtin_memset_local_pair.c` plus
  `builtin_memset_local_i32_array.c`
- Watchouts:
  direct `memcpy` and `memset` calls whose operands arrive as mixed-shape
  pointers, non-local bases, unsupported local-array/subobject forms, or
  non-zero fills still fall back today; keep follow-on work in shared
  semantic lowering rather than target- or testcase-shaped handling
- Proof:
  `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
  passed, including
  `backend_codegen_route_x86_64_builtin_memset_local_pair_observe_semantic_bir`
  and
  `backend_codegen_route_x86_64_builtin_memset_local_i32_array_observe_semantic_bir`
  alongside the existing backend semantic-call/runtime route coverage;
  `test_after.log` preserved

## Suggested Next

- extend semantic `memcpy` lowering from matching local aggregate objects and
  local scalar arrays plus direct zero-fill `memset` lowering to the next
  nearby shared local subobject or mixed-shape pointer family that still
  stays semantic and planner-honest
- if that next runtime-memory shape needs new semantic BIR surface, keep that
  surface shared and planner-honest rather than encoding direct target
  behavior
