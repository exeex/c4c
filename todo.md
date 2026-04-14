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
  `src/backend/lowering/lir_to_bir_module.cpp` on simple by-value aggregate
  returns, hidden sret-style call/result materialization, and aggregate-result
  copies through the existing local leaf-slot model
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
