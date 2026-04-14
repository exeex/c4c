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
  subobjects, matching local `i32` arrays, and mixed local aggregate/array
  leaf views, plus direct zero-fill `memset` calls for the local aggregate and
  nested local scalar-array families in `lir_to_bir`; the next packet should
  stay on adjacent runtime/call families instead of drifting into `prepare`
- packet rule:
  the first executor packet must change shared semantic lowering, not just add
  new test observers or rename unsupported cases
- proof expectation:
  choose a proving subset from backend-route or internal backend cases that
  exercises one semantic call/runtime family with a fresh build before broader
  validation
- Just Finished:
  generalized direct local `memcpy` lowering around shared ordered leaf views
  so matching local aggregate slots and local scalar-array views can copy
  across the same semantic route, then added backend-route proof with
  `builtin_memcpy_local_i32_array_to_pair.c` for local `i32` array to local
  pair copies without falling back to `memcpy`
- Watchouts:
  the new mixed local `memcpy` route still requires matching ordered leaf
  offsets, scalar types, and exact byte size, so padded shape mismatches,
  non-local bases, unsupported local-array/subobject forms, and non-zero
  `memset` calls still fall back today; keep follow-on work in shared
  semantic lowering rather than target- or testcase-shaped handling
- Proof:
  `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
  passed, including
  `backend_codegen_route_x86_64_builtin_memcpy_local_i32_array_to_pair_observe_semantic_bir`,
  `backend_codegen_route_x86_64_builtin_memcpy_nested_i32_array_field_observe_semantic_bir`,
  `backend_codegen_route_x86_64_builtin_memset_nested_i32_array_field_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`,
  and
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`;
  the canonical regression comparison against `test_before.log` also passed
  with `before: passed=28 failed=0 total=28` and
  `after: passed=29 failed=0 total=29`; `test_after.log` preserved

## Suggested Next

- extend adjacent semantic runtime-memory proof to the reverse or nested
  mixed-shape local `memcpy` family, such as local aggregate-to-array or
  aggregate-contained mixed local subobjects that now share the ordered
  leaf-view route, before moving on to a different runtime family
- if that next runtime-memory shape needs new semantic BIR surface, keep that
  surface shared and planner-honest rather than encoding direct target
  behavior
