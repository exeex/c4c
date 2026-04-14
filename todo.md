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
- 2026-04-14 temporary executor packet extension:
  split the `phi/select/cfg` lowering lane out of
  `src/backend/lowering/lir_to_bir_module.cpp` as its own functional slice
  without changing the active backend capability packet
- 2026-04-14 temporary executor packet result:
  `src/backend/lowering/lir_to_bir_cfg.cpp` now owns block-lookup,
  canonical-select-chain, phi-plan collection, and canonical-select lowering;
  `src/backend/lowering/lir_to_bir_module.cpp` keeps the branch-family/module
  flow and non-cfg lowering helpers
- 2026-04-14 temporary proof result:
  the same delegated backend proof command passes after the cfg split, and
  the proof log is `test_after.log`
- 2026-04-14 temporary executor packet extension:
  split scalar/value lowering and function-signature lowering out of
  `src/backend/lowering/lir_to_bir_module.cpp` into dedicated functional
  slices without changing the active backend capability packet
- 2026-04-14 temporary executor packet result:
  `src/backend/lowering/lir_to_bir_scalar.cpp` now owns shared scalar
  type/cast/binop/compare/select-chain lowering, and
  `src/backend/lowering/lir_to_bir_calling.cpp` now owns param/return
  signature lowering plus declaration helpers; `src/backend/lowering/lir_to_bir.hpp`
  exposes the cross-file API and `src/backend/lowering/lir_to_bir_module.cpp`
  keeps orchestration plus the still-unsplit aggregate/memory helpers
- 2026-04-14 temporary proof result:
  the same delegated backend proof command passes after the scalar/calling
  split, and the proof log is `test_after.log`
- 2026-04-14 temporary executor packet extension:
  split aggregate layout/slot/materialization helpers out of
  `src/backend/lowering/lir_to_bir_module.cpp` into a dedicated aggregate
  slice without changing the active backend capability packet
- 2026-04-14 temporary executor packet result:
  `src/backend/lowering/lir_to_bir_aggregate.cpp` now owns byval aggregate
  layout discovery, aggregate-param collection, local aggregate leaf-slot
  declaration, aggregate slot-to-slot copying, aggregate slot-to-pointer
  copying, and aggregate param alias materialization; `lir_to_bir.hpp`
  exposes the cross-file aggregate API and `lir_to_bir_module.cpp` keeps the
  remaining module flow plus the still-unsplit memory helpers
- 2026-04-14 temporary proof result:
  the same delegated backend proof command passes after the aggregate split,
  and the proof log is `test_after.log`
- 2026-04-14 temporary executor packet extension:
  split the remaining local/global memory, address, pointer, and call-memory
  lowering lane out of `src/backend/lowering/lir_to_bir_module.cpp` into a
  dedicated memory slice without changing the active backend capability packet
- 2026-04-14 temporary executor packet result:
  `src/backend/lowering/lir_to_bir_memory.cpp` now owns
  `lower_scalar_or_local_memory_inst(...)` plus the global-address, local-slot,
  GEP, pointer-array, load/store, memset, and call-memory helper cluster;
  `src/backend/lowering/lir_to_bir_module.cpp` now keeps branch-family/module
  orchestration, `lir_to_bir_aggregate.cpp` owns the shared
  `lower_byval_aggregate_layout(...)` helper, and `lir_to_bir.hpp` exposes the
  final cross-file memory API used by the split
- 2026-04-14 temporary proof result:
  the same delegated backend proof command passes after the memory split with
  `12 / 12` `^backend_` tests green, and the proof log is `test_after.log`
- 2026-04-14 temporary executor packet extension:
  refactor function-scope `lir_to_bir` lowering around a `BirFunctionLowerer`
  class so the function body path no longer routes through
  `lir_to_bir_detail::...` helpers
- 2026-04-14 temporary executor packet result:
  `src/backend/lowering/lir_to_bir.hpp` now declares `BirFunctionLowerer`,
  `src/backend/lowering/lir_to_bir_module.cpp` reduces function lowering to
  class orchestration, and the split scalar/calling/aggregate/cfg/memory
  implementation files now define function-scope lowering as
  `BirFunctionLowerer::...` methods, including the former
  `lower_scalar_or_local_memory_inst(...)` dispatcher
- 2026-04-14 temporary proof result:
  the same delegated backend proof command passes after the
  `BirFunctionLowerer` refactor with `12 / 12` `^backend_` tests green, and
  the proof log is `test_after.log`
- 2026-04-14 temporary executor packet extension:
  continue the `BirFunctionLowerer` class refactor by migrating the remaining
  function-scope anonymous-namespace helpers in the scalar/calling/aggregate/
  memory lowering slices onto `BirFunctionLowerer` as class methods or
  class-scoped statics, without changing backend behavior or the active proof
  surface
- 2026-04-14 temporary executor packet result:
  `src/backend/lowering/lir_to_bir_scalar.cpp`,
  `src/backend/lowering/lir_to_bir_calling.cpp`,
  `src/backend/lowering/lir_to_bir_aggregate.cpp`, and
  `src/backend/lowering/lir_to_bir_memory.cpp` no longer keep their
  function-scope lowering helpers in anonymous namespaces; the remaining
  scalar integer/cast support, void-param sentinel, aggregate param-slot
  naming, and memory/address/GEP/pointer-array support helpers now live on
  `BirFunctionLowerer`, and the dead duplicate
  `canonicalize_compare_return_alias(...)` helper was removed from
  `lir_to_bir_memory.cpp`
- 2026-04-14 temporary proof result:
  the same delegated backend proof command passes after the helper migration
  with `12 / 12` `^backend_` tests green, and the proof log is
  `test_after.log`
- 2026-04-14 temporary executor packet extension:
  continue the `BirFunctionLowerer` ownership cleanup by moving the
  function-only helper types and maps that are now used exclusively by the
  function-body lowering path out of `lir_to_bir_detail` and onto
  `BirFunctionLowerer`, without changing backend behavior or the active proof
  surface
- 2026-04-14 temporary executor packet result:
  `src/backend/lowering/lir_to_bir.hpp` no longer keeps
  `CompareExpr`/`CompareMap`, `BlockLookup`, `BranchChain`,
  `PhiLoweringPlan`/`PhiBlockPlanMap`, `AggregateParamInfo`/
  `AggregateParamMap`, `LoweredReturnInfo`, or
  `AggregateValueAliasMap` under `lir_to_bir_detail`; those function-scope
  ownership types now live on `BirFunctionLowerer`, and the split
  scalar/calling/aggregate/cfg/memory implementation files were updated to
  use the class-owned types directly while leaving truly shared utility types
  in `lir_to_bir_detail`
- 2026-04-14 temporary proof result:
  the same delegated backend proof command passes after the type-ownership
  migration with `12 / 12` `^backend_` tests green, and the proof log is
  `test_after.log`
- 2026-04-14 temporary executor packet extension:
  continue shrinking `lir_to_bir_detail` by moving the function-body
  memory-lane ownership types and maps that now only serve
  `BirFunctionLowerer` onto the class, without changing backend behavior or
  the active proof surface
- 2026-04-14 temporary executor packet result:
  `src/backend/lowering/lir_to_bir.hpp` no longer keeps the
  function-body-only memory/address ownership types under `lir_to_bir_detail`;
  `BirFunctionLowerer` now directly owns `GlobalPointerSlotKey`, the
  local/global pointer-array and aggregate-array access structs and maps, the
  local aggregate/local array storage structs and maps, and the related
  global/local address and pointer map aliases, while
  `src/backend/lowering/lir_to_bir_memory.cpp` now references the class-owned
  types directly
- 2026-04-14 temporary proof result:
  the same delegated backend proof command passes after the memory-lane
  type-ownership migration with `12 / 12` `^backend_` tests green, and the
  proof log is `test_after.log`
- 2026-04-14 temporary executor packet extension:
  move the call/signature parsing helpers that are genuinely owned by the
  `lir_to_bir` lowering path off the legacy `call_decode.*` dependency and
  onto `BirFunctionLowerer`, while leaving non-`lir_to_bir` matcher users on
  the legacy wrapper layer
- 2026-04-14 temporary executor packet result:
  `src/backend/lowering/lir_to_bir_calling.cpp` now defines
  `BirFunctionLowerer::lower_minimal_scalar_type(...)`,
  `parse_typed_call(...)`, `parse_direct_global_typed_call(...)`, and
  `parse_function_signature_params(...)`; the
  `lir_to_bir_calling/aggregate/memory` lowering slices now use those class
  methods directly, `lir_to_bir_globals.cpp` carries its own nonminimal-global
  screen instead of depending on `call_decode.*`, and `call_decode.cpp`
  remains as a legacy API wrapper that forwards its typed-call/signature parse
  entry points to the moved `BirFunctionLowerer` implementation
- 2026-04-14 temporary proof result:
  the same delegated backend proof command passes after the `call_decode`
  ownership split with `12 / 12` `^backend_` tests green, and the proof log
  is `test_after.log`

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
