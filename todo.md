# Backend Reboot From BIR Spine

Status: Active
Source Idea: ideas/open/46_backend_reboot_bir_spine.md
Source Plan: plan.md

## Lifecycle Repair Decision

- 2026-04-14 supervisor route checkpoint after backlog-item-2 scouting:
  keep the same active source idea and active runbook; do not close, split,
  or switch ideas
- backlog item 2 is now parked at the accepted non-variadic signature
  baseline:
  the checked-in x86 semantic-BIR sentinels already cover scalar params,
  aggregate `byval`, hidden `sret`, direct/indirect function-pointer calls,
  direct object-address pointer args, loaded global-pointer args/returns, and
  aggregate function-pointer call-through
- scouting result:
  the only nearby non-BIR misses are
  `tests/backend/case/nested_member_pointer_array.c`, which belongs to local
  pointer-bearing address formation, and
  `tests/backend/case/variadic_sum2.c`,
  `tests/backend/case/variadic_pair_second.c`, and
  `tests/backend/case/variadic_double_bytes.c`, which belong to later
  ABI/variadic work rather than the active non-variadic signature lane
- lifecycle decision:
  re-sequence active executor dispatch onto backlog item 3, broaden local
  memory and address formation
- execution state:
  ready for a bounded backlog-item-3 executor packet centered on shared local
  object/address semantics, starting from
  `tests/backend/case/nested_member_pointer_array.c`
- next lifecycle expectation:
  do not send another backlog-item-2 packet whose main effect is more
  signature-lane sentinel churn unless a fresh honest non-ABI signature seam
  appears
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
  backlog item 3, broaden local memory and address formation
- current packet focus:
  `src/backend/lowering/lir_to_bir_memory.cpp` /
  `src/backend/lowering/lir_to_bir_types.cpp` on shared local object/address
  semantics for pointer-bearing local aggregates, starting with
  `nested_member_pointer_array.c`
- proving surface:
  use `tests/backend/case/nested_member_pointer_array.c` as the first honest
  miss for this lane; keep `tests/backend/case/local_array.c` as nearby
  regression coverage rather than expanding signature-only sentinels again
- packet rule:
  do not spend the next packet on more backlog-item-2 harness churn or on
  variadic ABI work
- 2026-04-14 executor packet extension:
  shared local object/address lowering now accepts plain local aggregate base
  pointers as honest `ptr` values, keeps constant-offset pointer GEP chains as
  pointer-address metadata, and materializes pointer-address loads through
  scratch local slots so `struct Outer *` / nested pointer-member dereference
  routes can stay on semantic BIR instead of falling back to LLVM text
- 2026-04-14 executor packet result:
  `tests/backend/case/nested_member_pointer_array.c` now reaches semantic BIR
  on x86_64 with `bir.load_local ... , addr %p.o` for the outer field load,
  `bir.load_local ... , addr %t1+4` for the nested array element load, and a
  direct semantic caller `bir.call i32 get_second(ptr %lv.outer)`; nearby
  `tests/backend/case/local_array.c` remains on its prior local-slot route
- 2026-04-14 proof extension:
  add
  `backend_codegen_route_x86_64_nested_member_pointer_array_observe_semantic_bir`
  as a dual backend-route sentinel that proves the new nested pointer-bearing
  aggregate route while keeping `local_array.c` as nearby regression coverage
- 2026-04-14 proof result:
  the delegated proof command passes with the new nested-member/local-array
  backend route sentinel included, and `test_after.log` is the proof log path
- next adjacent local-address gap:
  this slice makes constant-offset pointer-param/member dereference stay on
  semantic BIR, but broader dynamic-index local-address formation and later
  ABI/legalization work remain outside the current packet
- 2026-04-14 executor packet extension:
  addressed-pointer metadata now preserves the originating aggregate storage
  shape for semantic local-address roots, and bounded dynamic scalar member
  arrays lower as addressed load ladders plus select chains instead of
  falling back to LLVM `getelementptr` / `load` text when the index is
  non-constant
- 2026-04-14 executor packet result:
  `tests/backend/case/local_dynamic_member_array.c` now reaches semantic BIR
  on x86_64 with three addressed loads from `%p.p`, a bounded `bir.select`
  ladder on the lowered dynamic index, and a semantic caller
  `bir.call i32 get_at(ptr %lv.p, i32 2)`; nearby
  `nested_member_pointer_array.c` and `local_array.c` stay on their accepted
  local-address routes
- 2026-04-14 proof extension:
  add
  `backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir`
  as a dual backend-route sentinel that proves bounded dynamic local member
  array addressing while keeping `local_array.c` as nearby regression coverage
- 2026-04-14 proof result:
  the delegated proof command now passes `16 / 16` backend tests with the new
  dynamic-member/local-array route sentinel included, and `test_after.log` is
  the proof log path
- next adjacent local-address gap:
  bounded dynamic scalar member-array loads now stay on semantic BIR, but
  dynamic addressed stores and broader non-scalar / later ABI-legalized local
  address flows remain outside the current packet
- 2026-04-14 executor packet extension:
  bounded dynamic scalar addressed stores now stay on semantic BIR too when a
  pointer-param/member-array path carries the local object root as addressed
  pointer metadata; lowering materializes addressed load/select/store ladders
  at fixed offsets instead of falling back to LLVM `getelementptr` / `store`
  text for the non-constant index
- 2026-04-14 executor packet result:
  `tests/backend/case/local_dynamic_member_array_store.c` now reaches semantic
  BIR on x86_64 with three addressed loads from `%p.p`, per-element
  `bir.select` updates against the lowered index, and addressed
  `bir.store_local ... , addr %p.p+N` writes in `set_at`; nearby
  `local_dynamic_member_array.c` and `nested_member_pointer_array.c` stay on
  their accepted local-address routes
- 2026-04-14 proof extension:
  add
  `backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir`
  as the bounded dynamic-store sentinel while keeping the earlier
  read-oriented local-address sentinels in the same proof subset
- 2026-04-14 proof result:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_x86_64_(nested_member_pointer_array|local_dynamic_member_array|local_dynamic_member_array_store).*semantic_bir'`
  now passes `3 / 3`, and `test_after.log` is the proof log path
- next adjacent local-address gap:
  bounded dynamic scalar member-array stores now stay on semantic BIR for
  addressed pointer-value roots, but the same store semantics are still not
  generalized to direct local aggregate-slot roots or broader non-scalar /
  later ABI-legalized local address flows
- 2026-04-14 executor packet extension:
  bounded dynamic scalar member-array stores now stay on semantic BIR for
  direct local aggregate-slot roots too when the local array base is already
  represented as a leaf-slot pointer; lowering recognizes the canonical
  local-root zero-leading GEP shape, records bounded direct-local aggregate
  access, and materializes per-element load/select/store ladders back into the
  real `%lv.p.+offset` slots instead of falling back to LLVM `getelementptr` /
  `store` text
- 2026-04-14 executor packet result:
  `tests/backend/case/local_direct_dynamic_member_array_store.c` now reaches
  semantic BIR on x86_64 with three direct local-slot loads from `%lv.p.0`,
  `%lv.p.4`, and `%lv.p.8`, per-element `bir.select` updates against the
  lowered dynamic index, and matching `bir.store_local %lv.p.+offset` writes
  in `main`; nearby `local_dynamic_member_array_store.c`,
  `local_dynamic_member_array.c`, and `nested_member_pointer_array.c` stay on
  their accepted local-address routes
- 2026-04-14 proof extension:
  add
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
  as the direct-local bounded dynamic-store sentinel while keeping the earlier
  addressed-root and read-oriented local-address sentinels in the same proof
  subset
- 2026-04-14 proof result:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_x86_64_(nested_member_pointer_array|local_dynamic_member_array|local_dynamic_member_array_store|local_direct_dynamic_member_array_store).*semantic_bir'`
  now passes `4 / 4`, and `test_after.log` is the proof log path
- next adjacent local-address gap:
  bounded dynamic scalar member-array stores now stay on semantic BIR for
  both addressed pointer-value roots and direct local aggregate-slot roots,
  but direct-local dynamic loads plus broader non-scalar / later
  ABI-legalized local address flows remain outside the current packet
- previous backlog-item-2 history below is accepted baseline only; do not mine
  it for the next packet unless a fresh non-ABI signature seam is named
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
- 2026-04-14 executor packet extension:
  pointer-valued global loads now stay on the semantic signature lane too when
  they flow through a local pointer slot into a direct call arg or pointer
  return, instead of leaving an undefined SSA name at the call edge
- 2026-04-14 executor packet result:
  `src/backend/lowering/lir_to_bir_memory.cpp` now keeps tracked local pointer
  loads honest: function symbols still fold to direct aliases when possible,
  but other tracked pointer values now emit a real `bir.load_local ptr`
  alongside preserved address provenance so `gp`-sourced locals lower as
  `load_global @gp -> load_local %lv.p -> bir.call ... %t1` instead of
  `bir.call ... %t1` with no defining instruction
- 2026-04-14 proof extension:
  add
  `backend_codegen_route_x86_64_pointer_param_global_pointer_value_arg_observe_semantic_bir`
  and
  `backend_codegen_route_x86_64_pointer_return_global_pointer_value_observe_semantic_bir`
  as signature-lane sentinels for pointer values loaded from global pointer
  objects before the call edge
- 2026-04-14 proof result:
  the delegated proof command now passes `14 / 14` backend tests with the two
  new loaded-global-pointer sentinels included, and `test_after.log` is the
  proof log path
- next adjacent signature-lane gap:
  this slice fixes loaded global pointer values that were previously emitting
  broken semantic BIR, but broader pointer-source coverage and downstream
  ABI/legalization work remain outside the current packet
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

- 2026-04-14 lifecycle checkpoint result:
  the temporary `lir_to_bir` shape-cleanup packet is complete, and follow-up
  scouting shows backlog item 2 has no remaining honest non-ABI executor
  packet on the current x86 semantic-BIR surface
- next adjacent follow-up:
  prepare a bounded backlog-item-3 executor packet on shared local
  object/address semantics, starting with pointer-bearing local aggregate
  addressing in `tests/backend/case/nested_member_pointer_array.c`
- route guard:
  do not pull `variadic_*` cases forward here, and do not spend the next
  packet only on more item-2 signature sentinels
- delegated proof command:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
