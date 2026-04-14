# Backend Reboot From BIR Spine

Status: Active
Source Idea: ideas/open/46_backend_reboot_bir_spine.md
Source Plan: plan.md

## Lifecycle Repair Decision

- 2026-04-14 plan-owner route checkpoint:
  keep the same active source idea and active runbook; do not close, split,
  or switch ideas
- direct prepare/BIR explicit-phi materialization in
  `src/backend/prepare/legalize.cpp` is now checkpointed rather than the
  current packet focus:
  five accepted executor slices plus
  `backend_prepare_phi_materialize` already cover reducible multi-incoming
  joins, return-only joins, successor-consumed joins,
  forwarded-successor joins, and conditional-successor joins
- lifecycle decision:
  park backlog item 1 at that accepted baseline and re-sequence active
  executor dispatch onto backlog item 2, harden params and function
  signatures
- execution state:
  ready for a bounded backlog-item-2 executor packet on the existing
  behavior-first proving surface
- next lifecycle expectation:
  do not resume backlog-item-1 executor work unless a fresh honest merge/phi
  seam can be named without reusing stale route scouting or merely extending
  the current reducible-phi harness
- supervisor action from this checkpoint:
  commit this lifecycle slice, then prepare the next executor packet from
  backlog item 2

## Current Active Item

- active route:
  semantic BIR remains the truth surface, `prepare` owns target legality, and
  target backends should ingest prepared BIR as the normal path
- current capability family:
  backlog item 2, harden params and function signatures
- current packet focus:
  `src/backend/lowering/lir_to_bir_module.cpp` on combined simple by-value
  aggregate params plus hidden sret-style aggregate returns, including direct
  param-to-return and call-through materialization through the existing local
  leaf-slot model
- 2026-04-14 executor packet extension:
  combined aggregate signature lowering now materializes byval aggregate
  params into entry local aggregate aliases so direct param-to-return and
  call-through aggregate routes lower through semantic BIR instead of falling
  back to raw LLVM aggregate signatures
- 2026-04-14 executor packet result:
  `src/backend/lowering/lir_to_bir_module.cpp` now lowers simple aggregate
  returns through semantic BIR as explicit hidden `ptr sret(size, align)`
  contracts, materializes aggregate call results in caller-owned local
  leaf-slot aggregates, and copies aggregate return values between local
  aggregate aliases and the sret pointer without falling back to raw LLVM IR
- 2026-04-14 direct x86 observation result:
  `tests/backend/case/aggregate_return_pair.c` now reaches semantic BIR on the
  truthful x86 backend route with `bir.func @make_pair(ptr sret(...)) -> void`
  plus a matching `bir.call void make_pair(ptr sret(...))`, instead of
  falling back to raw LLVM struct-return IR
- proving surface:
  keep `tests/backend/case/param_slot.c`,
  `tests/backend/case/param_member_array.c`, and
  `tests/backend/case/nested_param_member_array.c` as the params baseline, and
  add `tests/backend/case/aggregate_return_pair.c` as the first honest
  behavior-shaped aggregate-return observation surface
- remaining blocker:
  the truthful runtime x86 path still stops at prepared-BIR/toolchain output
  rather than native asm, and broader aggregate memory-copy / ABI legalization
  beyond the landed byval-entry plus aggregate-return sret slice remains
  outside this packet
- supporting proof surface:
  `backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir`
  and
  `backend_codegen_route_x86_64_aggregate_return_pair_observe_semantic_bir`
  now serve as the authorized narrow harness proofs for the aggregate
  signature slice, while
  `backend_codegen_route_x86_64_param_slot_observes_prepared_bir` remains the
  earlier scalar signature sentinel
- active proof extension:
  add `backend_codegen_route_x86_64_aggregate_param_return_pair_observe_semantic_bir`
  as the first combined byval-param plus sret-return sentinel so the active
  packet proves more than separate one-sided aggregate signature moves
- 2026-04-14 executor packet extension:
  constant function-pointer aggregate call sites now stay on the semantic
  signature lane too: backend call decoding canonicalizes `ptr byval(...)`
  call-site args back to their aggregate param types, and lowering carries
  known function-symbol pointer aliases through the call edge instead of
  falling back to raw LLVM aggregate signatures
- 2026-04-14 executor packet result:
  `tests/backend/case/indirect_aggregate_param_return_pair.c` now proves the
  same aggregate param+return rule through a local function-pointer call site;
  semantic BIR shows `bir.call void @id_pair(ptr sret(...), ptr byval(...))`
  instead of LLVM `%struct.Pair (%struct.Pair) %tN(...)` fallback text
- 2026-04-14 proof result:
  the exact backend proof command now passes `7 / 7` backend tests, including
  the new indirect aggregate function-pointer sentinel; semantic BIR for both
  `aggregate_param_return_pair.c` and
  `indirect_aggregate_param_return_pair.c` shows `id_pair` lowering as
  `ptr sret(...)` plus `ptr byval(...)` instead of raw `%struct.Pair`
  signatures/calls, and the proof log is `test_after.log`
- 2026-04-14 executor packet extension:
  direct global-call lowering now carries plain `ptr` args too, including
  function-symbol operands passed into signature-lane helper params, instead
  of rejecting non-integer, non-byval direct-call args and forcing whole
  modules back to raw LLVM
- 2026-04-14 executor packet result:
  the minimal `apply(inc, 4)` function-pointer-param route now reaches
  semantic BIR as `bir.call i32 apply(ptr @inc, i32 4)`, and the aggregate
  parameterized function-pointer case now lowers end to end with
  `bir.func @use(ptr %p.fn, ptr byval(...)) -> i32`, an indirect
  `bir.call void %p.fn(ptr sret(...), ptr byval(...))`, and a matching direct
  caller `bir.call i32 use(ptr @id_pair, ptr byval(...))`
- 2026-04-14 proof extension:
  add
  `backend_codegen_route_x86_64_function_pointer_param_direct_arg_observe_semantic_bir`
  as the minimal pointer-arg sentinel and
  `backend_codegen_route_x86_64_aggregate_param_return_pair_fn_param_observe_semantic_bir`
  as the honest aggregate function-pointer-param route sentinel for this
  signature slice
- regression sentinels:
  keep the `two_arg_*` helper family as runtime and route sentinels, not as
  the primary proof source for this lane
- packet rule:
  do not treat trivial native-asm output on helper cases as proof of semantic
  signature progress, and do not spend the next packet on harness-only churn

## Parked Backlog Item 1 Baseline

- accepted backlog-item-1 state:
  semantic phi lowering is landed in the shared lowering route, and `prepare`
  now materializes reducible explicit-phi trees through the dedicated direct
  harness in `tests/cpp/internal/backend_prepare_phi_materialize_test.cpp`
- current direct-harness coverage:
  multi-incoming funnels, return-only joins, successor-consumed joins,
  forwarded-successor joins, and conditional-successor joins
- accepted proof command:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
- accepted subset envelope:
  `420 total / 209 passed / 211 failed`
- route rule:
  treat the explicit-phi prepare harness as regression coverage for later work,
  not as the default next packet source unless a new honest merge seam is
  identified first

## Immediate Target

- next executor packet should make a real code move in the backlog-item-2
  signature lane, not extend parked phi coverage
- preferred first proof surface remains
  `param_slot`, `param_member_array`, `nested_param_member_array`, and
  `aggregate_return_pair`
- default proving command for the next backend packet remains:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
- 2026-04-14 proof result:
  the exact backend proof command above now passes `5 / 5` backend tests,
  including the new
  `backend_codegen_route_x86_64_aggregate_return_pair_observe_semantic_bir`
  harness; direct x86 semantic-BIR output for `aggregate_return_pair.c` now
  shows the hidden sret contract instead of LLVM struct-return fallback, and
  the proof log is `test_after.log`
- if runtime behavior alone cannot expose the signature change cleanly,
  authorize at most one narrow harness packet that supports the same code move
  without regressing supported prepared-BIR emission
