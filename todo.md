# Backend Reboot From BIR Spine

Status: Active
Source Idea: ideas/open/46_backend_reboot_bir_spine.md
Source Plan: plan.md

## Current Active Item

- active route:
  semantic BIR remains the truth surface, `prepare` owns target legality, and
  target backends should eventually ingest prepared BIR only
- current capability family:
  backlog item 1, generalize CFG merge and `phi`
- current packet focus:
  first real shared `phi` / CFG merge semantics slice beyond the current
  synthetic local-slot materialization in
  `src/backend/lowering/lir_to_bir_module.cpp` and owned BIR files, using the
  ref backend as the ordering guide:
  shared generation owns CFG semantics before later stack/regalloc phases, and
  later prepare work should consume merge-attributed results rather than
  reconstructing them inside call lowering
- why now:
  the post-`b21fdb42` route drifted into backlog-item-2 proving-surface churn
  without changing lowering code, while the ref backend structure makes clear
  that `phi` belongs before more signature/call expansion
- proving-surface checkpoint:
  2026-04-14 focused proof and follow-up sampling showed that
  `single_param_select_eq` plus the full split-predecessor
  `*_post_add_sub_add` family are not honest next packets by themselves:
  they already lower through the current route, so more named-stem promotion
  there would be route churn rather than new merge semantics
- packet rule:
  do not accept more `todo` / `InternalTests.cmake` churn as proxy progress;
  stale unsupported-test promotion for the split-predecessor
  `*_post_add_sub_add` stems is cleanup only, not backlog-item-1 progress;
  the next accepted packet must change lowering/BIR code and move the shared
  merge contract forward rather than one more testcase-surface promotion

## Immediate Target

- keep backlog item 2 params/signatures and backlog item 5 call lowering as
  sentinels only until backlog item 1 has a new code change
- treat the split-predecessor `*_post_add_sub_add` unsupported stems as stale
  proving/harness debt, not as the next executor target:
  current lowering already reaches semantic BIR for that family
- define the first semantic contract in owned lowering files:
  which merges still fold to `bir.select`, and which merges stay explicit as
  shared merge semantics for later prepare work instead of today’s synthetic
  `bir.store_local` / `bir.load_local` phi-slot materialization
- start from one ordinary merge shape or explicit representation gap that is
  still missing after the stale split-predecessor family is discounted:
  prefer a non-diamond or predecessor-attributed merge where later phases
  should consume merge meaning directly rather than rediscovering it from
  synthetic slots
- keep call lowering out of scope for this packet except as a consumer-side
  sentinel:
  no new call-specific merge reconstruction is allowed
- keep helper params, indirect-call provenance, globals, and runtime intrinsics
  out of scope for this packet
- if a proof case is needed, choose one merge case plus `branch_if_eq` and one
  existing call-lane sentinel; do not use `two_arg_*` helper output mode as the
  primary evidence surface

## Done Condition For The Active Packet

- lowering/BIR files gain the first real non-ternary shared merge capability
- at least one merge shape broader than the existing diamond-select lane no
  longer relies on route churn or raw-LIR fallback
- `branch_if_eq.c` still lowers cleanly
- existing call-lane sentinels stay green without new call-specific merge logic
- no direct route, rendered-text matcher, or tiny named-case special path is
  introduced

## Proof Routing Note

- supervisor should choose a narrow build-plus-test command anchored on one
  merge case or merge-observation surface that is still a real gap after the
  stale split-predecessor `*_post_add_sub_add` family is excluded, plus
  `branch_if_eq` and one call-lane sentinel as non-regression checks
- do not reuse helper-only param surfaces or arithmetic-route regex churn as
  the primary proving subset for the next executor packet
- if existing tests still cannot honestly expose the new merge capability, the
  next lifecycle repair may authorize one minimal observation/harness packet,
  but only after a real lowering attempt in the owned files
- stale unsupported split-predecessor tests may be cleaned up later, but that
  cleanup must follow a code-moving packet or be explicitly treated as harness
  debt rather than accepted capability progress

## Latest Packet Progress

- 2026-04-14 review and supervisor inspection agreed that the post-`b21fdb42`
  execution drifted into proving-surface churn instead of backlog-item-1 code
  progress:
  recent changes after that checkpoint touched lifecycle/test wiring rather
  than lowering/BIR files
- 2026-04-14 supervisor validation then showed the currently unsupported
  split-predecessor `*_post_add_sub_add` family is stale as a proof target:
  all five `backend_codegen_route_riscv64_*_asm_unsupported` cases now fail
  because the backend already emits semantic BIR for them, including
  `add_phi`, `mixed_affine`, `deeper_then_mixed`, `deeper_affine`, and
  `mixed_then_deeper`
- ref-based route confirmation recorded:
  `ref/claudes-c-compiler/src/backend/generation.rs` handles CFG semantics in
  the shared backend pipeline before later target work, while
  `ref/claudes-c-compiler/src/backend/stack_layout/mod.rs` and
  `stack_layout/slot_assignment.rs` treat multi-definition values as a
  downstream consequence of phi elimination rather than as a call-lane concern
- next packet therefore returns to backlog item 1:
  first real shared `phi` / CFG merge semantics move in owned lowering/BIR
  files beyond the already-broader slotized merge family, with params/calls
  kept as sentinels only until merge semantics move forward in code
