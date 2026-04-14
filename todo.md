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
  `memcpy` lowering for matching local aggregates, nested local scalar-array
  subobjects, and matching local `i32` arrays, plus direct zero-fill `memset`
  calls for the same local aggregate and nested local scalar-array families in
  `lir_to_bir`; the next packet should stay on adjacent runtime/call families
  instead of drifting into `prepare`
- packet rule:
  the first executor packet must change shared semantic lowering, not just add
  new test observers or rename unsupported cases
- proof expectation:
  choose a proving subset from backend-route or internal backend cases that
  exercises one semantic call/runtime family with a fresh build before broader
  validation
- Just Finished:
  generalized nested local scalar-array views so direct `memcpy` and zero-fill
  `memset` calls on aggregate-contained `i32` arrays stay on semantic BIR
  without displacing the existing direct dynamic local member-array semantic
  route, then added backend-route proof with
  `builtin_memcpy_nested_i32_array_field.c` and
  `builtin_memset_nested_i32_array_field.c`
- Watchouts:
  nested aggregate field zero-fill was already semantic; the actual miss was
  nested local scalar arrays not entering the shared local-array base map, so
  follow-on GEPs that already have scalar-slot ownership must keep preferring
  that route even when the same pointer also gains a shared local-array base,
  and direct `memcpy`/`memset` calls whose operands arrive as mixed-shape
  pointers, non-local bases, unsupported local-array/subobject forms, or
  non-zero fills still fall back today; keep follow-on work in shared
  semantic lowering rather than target- or testcase-shaped handling
- Proof:
  `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
  passed, including
  `backend_codegen_route_x86_64_builtin_memcpy_nested_i32_array_field_observe_semantic_bir`,
  `backend_codegen_route_x86_64_builtin_memset_nested_i32_array_field_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`,
  and
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`;
  `test_after.log` preserved

## Suggested Next

- extend semantic `memcpy` lowering from matching local aggregate objects and
  direct zero-fill `memset` lowering from matching local aggregate objects and
  nested local scalar-array views to the next nearby shared local subobject or
  mixed-shape pointer family that still stays semantic and planner-honest
- if that next runtime-memory shape needs new semantic BIR surface, keep that
  surface shared and planner-honest rather than encoding direct target
  behavior
