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
  add `backend_codegen_route_x86_64_pointer_return_direct_observe_semantic_bir`
  and
  `backend_codegen_route_x86_64_pointer_return_fn_param_observe_semantic_bir`
  as plain-pointer signature-lane sentinels so x86_64 keeps both direct and
  indirect pointer-return calls on semantic BIR instead of dropping back to
  raw LLVM call text
- 2026-04-14 executor packet result:
  `tests/backend/case/pointer_return_direct.c` and
  `tests/backend/case/pointer_return_fn_param.c` now lock the pointer-return
  signature lane on param-routed shapes the backend already lowers: semantic
  BIR shows `bir.call ptr id_ptr(ptr %p.p)` for the direct helper and
  `bir.call ptr %p.fn(ptr %p.p)` for the function-parameter helper instead of
  LLVM `call ptr (...)` fallback text
- 2026-04-14 proof result:
  the delegated proof command passes with the two new pointer-return backend
  route sentinels included, and `test_after.log` is the proof log path
- 2026-04-14 executor packet extension:
  plain `ptr` call arguments sourced from direct global/object addresses now
  stay on the semantic signature lane too, instead of forcing direct-global
  call sites such as `consume(&g)` back to raw LLVM call text
- 2026-04-14 executor packet result:
  `tests/backend/case/pointer_param_direct_global_arg.c` now proves the
  object-address pointer-arg route on x86_64: semantic BIR shows
  `bir.call i32 consume(ptr @g)` in `main` instead of LLVM
  `call i32 (ptr) @consume(ptr @g)` fallback text
- 2026-04-14 proof result:
  the delegated proof command now passes `12 / 12` backend tests with the new
  `backend_codegen_route_x86_64_pointer_param_direct_global_arg_observe_semantic_bir`
  sentinel included, and `test_after.log` is the proof log path
- next adjacent signature-lane gap:
  this slice closes the direct `@g`-style object-address pointer-arg hole, but
  broader pointer-source coverage and downstream ABI/legalization work remain
  outside the current packet
- regression sentinels:
  keep the `two_arg_*` helper family as runtime and route sentinels, not as
  the primary proof source for this lane
- packet rule:
  do not treat trivial native-asm output on helper cases as proof of semantic
  signature progress, and do not spend the next packet on harness-only churn
- 2026-04-14 temporary executor packet:
  refactor `src/backend/lowering/lir_to_bir*` into clearer functional slices
  ahead of the active backlog-item-2 code path, without changing backend
  capability or test scope
- 2026-04-14 temporary executor packet result:
  `src/backend/lowering/lir_to_bir.hpp` now exposes the shared cross-file
  lowering API for agent discovery, shared type/layout helpers moved into
  `src/backend/lowering/lir_to_bir_types.cpp`, global/address lowering moved
  into `src/backend/lowering/lir_to_bir_globals.cpp`, and
  `src/backend/lowering/lir_to_bir_module.cpp` now keeps the remaining
  module/function lowering flow instead of owning those helper buckets too;
  `CMakeLists.txt` was updated to build the new files
- 2026-04-14 temporary proof result:
  the delegated proof command passes `12 / 12` `^backend_` tests after the
  refactor, and the proof log is `test_after.log`
- 2026-04-14 temporary refactor baseline:
  the `src/backend/lowering/lir_to_bir*` split is now complete and accepted:
  cfg/scalar/calling/aggregate/memory lowering each live in their own slices,
  `BirFunctionLowerer` owns the function-body lowering path and its
  function-only helper methods/types, the memory-lane ownership structs/maps
  also live on `BirFunctionLowerer`, and the `lir_to_bir`-specific
  call/signature parsing helpers have been absorbed out of the legacy
  `call_decode.*` dependency into the `lir_to_bir` calling lane
- 2026-04-14 temporary refactor proof:
  every packet above was validated with the same delegated backend command and
  the accepted refactor baseline remains `12 / 12` passing `^backend_` tests
  with proof logged through `test_after.log`

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

- next executor packet is a structural follow-up on the same lowering lane:
  reduce `BirFunctionLowerer` header surface and regroup its methods into
  clearer internal buckets without changing backend behavior
- target the current pain point directly:
  split `BirFunctionLowerer` private API into more coherent method families
  such as calling/cfg/scalar/aggregate/memory helpers, and move declaration
  detail out of `src/backend/lowering/lir_to_bir.hpp` where that can be done
  without reopening the shared utility boundary
- keep this packet scoped to class shape only:
  do not restart capability scouting, do not broaden proof scope, and do not
  re-open the legacy `call_decode.*` layer unless the header regrouping
  strictly requires it
- default proving command for the next backend packet remains:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
