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
  the first explicit semantic `bir.phi` / CFG merge packet is now considered
  exhausted, and the active route is authorized to spend exactly one bounded
  follow-up packet on observation/harness work;
  that bounded slice is now landed:
  `src/backend/lowering/lir_to_bir_module.cpp` and owned BIR files already
  gained semantic phi lowering, `prepare` still owns the temporary phi-slot
  materialization needed by the normal backend-route output surface, and the
  compiler now has one minimal opt-in semantic-BIR observation path that can
  expose shared merge semantics before `prepare`; the next packet should
  return to code-moving backlog-item-1 work rather than more harness churn
- why now:
  supervisor follow-up confirmed that more of the remaining merge proving
  surface is stale harness debt rather than a real backlog-item-1 semantic
  gap:
  both `two_param_select_eq_predecessor_add_post_add` and
  `two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub`
  still sit under `asm_unsupported`, but already emit prepared BIR plus
  `semantic_phi` trailers
- proving-surface checkpoint:
  2026-04-14 focused proof and follow-up sampling showed that
  `single_param_select_eq` plus the full split-predecessor
  `*_post_add_sub_add` family are not honest next packets by themselves:
  they already lower through the current route, so more named-stem promotion
  there would be route churn rather than new merge semantics
  the 2026-04-14 executor slice then confirmed the current route-test harness
  observes prepared BIR, not semantic pre-prepare BIR:
  the new semantic `bir.phi` nodes are lowered back to the temporary
  local-slot form during `prepare`, so direct semantic-phi observation still
  needs either a dedicated harness or a different proof surface;
  supervisor re-check then expanded the stale-harness set to include
  `backend_codegen_route_riscv64_two_param_select_eq_predecessor_add_post_add_asm_unsupported`
  and
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_asm_unsupported`,
  both of which now fail because the backend already emits prepared BIR plus
  semantic-phi observation for those stems;
  an additional supervisor pass then ruled out the simplest remaining select
  inventory as an honest next code-moving packet too:
  both `backend_codegen_route_riscv64_single_param_select_eq_asm_unsupported`
  and `backend_codegen_route_riscv64_two_param_select_eq_asm_unsupported`
  already emit BIR, so the current backend-route inventory no longer exposes a
  remaining backlog-item-1 merge gap cleanly
- packet rule:
  do not accept more `todo` / `InternalTests.cmake` churn as proxy progress;
  stale unsupported-test promotion for the split-predecessor
  `*_post_add_sub_add` stems plus the newly confirmed
  `two_param_select_eq_predecessor_add_post_add` and
  `two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub`
  stems is harness cleanup only, not backlog-item-1 progress;
  the next accepted packet may be one minimal observation/harness slice only
  because the supervisor has now ruled out the existing backend-route
  inventory as an honest source of another immediate code-moving merge target;
  that harness slice must stay generic, bounded, and merge-semantic in scope
  rather than promoting one more named testcase surface

## Immediate Target

- the authorized observation/harness slice is now complete, so the next packet
  should return to one real code-moving backlog-item-1 merge target using the
  new semantic-BIR observation surface only as proof support
- keep backlog item 2 params/signatures and backlog item 5 call lowering as
  sentinels only until backlog item 1 has another real code change
- treat the split-predecessor `*_post_add_sub_add` stems plus the newly stale
  `two_param_select_eq_predecessor_add_post_add` and
  `two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub`
  stems as harness debt, not as the next executor target
- the next executor packet is now authorized to be one minimal observation /
  harness slice for backlog item 1:
  expose semantic BIR before prepare on one canonical `bir.select` surface and
  one explicit `bir.phi` surface without changing the normal prepared-BIR
  backend contract
- keep the harness target generic:
  prefer one small reusable observation path or test helper that can inspect
  semantic BIR for merge semantics by meaning, not a named-case printer hook
  or one-off testcase branch
- keep call lowering out of scope for the next code packet except as a
  consumer-side sentinel:
  no new call-specific merge reconstruction is allowed
- keep helper params, indirect-call provenance, globals, and runtime intrinsics
  out of scope for this repair
- for proof, anchor the harness on one canonical diamond-select case and one
  explicit merge case, then keep `branch_if_eq` and one existing call-lane
  sentinel as non-regression checks; do not use `two_arg_*` helper output mode
  as the primary evidence surface

## Done Condition For The Next Accepted Packet

- one bounded semantic-BIR observation path exists that can expose both a
  canonical `bir.select` merge and an explicit `bir.phi` merge before prepare
- the observation path does not require testcase-shaped branches, rendered-text
  whole-module matcher growth, or output-mode changes in the normal backend
  contract
- `branch_if_eq.c` still lowers cleanly
- existing call-lane sentinels stay green without new call-specific merge logic
- no direct route, rendered-text matcher, or tiny named-case special path is
  introduced

## Proof Routing Note

- supervisor should choose a narrow build-plus-test command anchored on the
  new semantic-BIR observation surface for one canonical select case and one
  explicit phi case, plus `branch_if_eq` and one call-lane sentinel as
  non-regression checks
- do not reuse helper-only param surfaces or arithmetic-route regex churn as
  the primary proving subset for the next executor packet
- this authorization is for one minimal observation/harness packet only;
  after it lands, the route should return to code-moving backlog-item-1 work
- stale unsupported split-predecessor tests may be cleaned up later, but that
  cleanup must follow a code-moving packet or be explicitly treated as harness
  debt rather than accepted capability progress
- because current `backend_codegen_route` tests observe prepared BIR, the next
  packet should add one minimal semantic-BIR observation surface rather than
  pretending the existing prepared-BIR route inventory still exposes a missing
  merge family cleanly

## Latest Packet Progress

- 2026-04-14 executor added one generic opt-in semantic-BIR observation path
  for backend route tests without changing the default prepared-BIR contract:
  `c4cll --backend-bir-stage semantic --codegen asm ...` now emits the raw
  semantic BIR produced before `prepare`, while the default `--codegen asm`
  route still prepares BIR exactly as before
- 2026-04-14 route coverage for that bounded harness packet now includes both
  a canonical select surface and an explicit phi surface via
  `backend_codegen_route_riscv64_single_param_select_eq_observes_semantic_bir`
  and
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_observes_semantic_bir`
  while keeping
  `backend_codegen_route_riscv64_branch_if_eq_defaults_to_bir` and
  `backend_codegen_route_riscv64_indirect_select_local_override_callee_call_defaults_to_bir`
  as non-regression sentinels
- 2026-04-14 exact delegated proof for the observation packet passed as
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_riscv64_(single_param_select_eq_observes_semantic_bir|two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_observes_semantic_bir|branch_if_eq_defaults_to_bir|indirect_select_local_override_callee_call_defaults_to_bir))$'`
  and wrote the result to `test_after.log`
- 2026-04-14 executor follow-up added one minimal semantic-merge observation
  surface without changing the prepared-BIR route contract:
  `src/backend/prepare/legalize.cpp` now preserves lowered-phi provenance on
  the temporary phi slot, and `src/backend/bir_printer.cpp` prints that
  preserved merge meaning as a `semantic_phi ... = bir.phi ...` trailer after
  the prepared BIR body
- 2026-04-14 route coverage for that observation surface landed on the active
  backlog-item-1 family via
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_observes_semantic_phi`,
  while focused non-regression re-checks also kept
  `backend_codegen_route_riscv64_branch_if_eq_defaults_to_bir` and
  `backend_codegen_route_riscv64_indirect_select_local_override_callee_call_defaults_to_bir`
  green
- 2026-04-14 exact delegated proof ran as
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  and wrote to `test_after.log`, but that broad backend subset still fails at
  branch scope across many existing route/runtime/toolchain cases, so this
  slice is locally proven on its owned route surface but not yet
  acceptance-ready from the supervisor-selected broad proof alone
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
- 2026-04-14 executor packet completed the first explicit semantic merge move
  in owned code:
  general branch-family lowering now emits `bir.phi` for top-of-block LIR
  `phi`, and `src/backend/prepare/legalize.cpp` lowers those semantic phi nodes
  back into today’s local-slot load/store form so existing prepared-BIR
  backend routes remain stable
- 2026-04-14 executor cleanup promoted the stale split-predecessor
  `*_post_add_sub_add` proving family from `asm_unsupported` to
  `defaults_to_bir`, because the current backend already lowers that family
  through prepared BIR; exact proof ran via
  `cmake --build --preset default` plus
  `ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_riscv64_branch_if_eq_defaults_to_bir|backend_codegen_route_riscv64_indirect_select_callee_call_defaults_to_bir|backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_affine_post_add_sub_add_(asm_unsupported|defaults_to_bir))$'`
  and wrote the passing result to `test_after.log`
- 2026-04-14 broader supervisor-side follow-up validation also passed for the
  twelve split-predecessor `defaults_to_bir` route tests covering the existing
  post-add/post-add-sub family plus the newly promoted `*_post_add_sub_add`
  family
- 2026-04-14 supervisor follow-up also confirmed that the remaining
  `backend_codegen_route_riscv64_two_param_select_eq_predecessor_add_post_add_asm_unsupported`
  and
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_asm_unsupported`
  stems are stale harness debt:
  both currently fail with `BACKEND_ASM_UNSUPPORTED_EXPECTED_FAIL` because the
  backend already emits prepared BIR plus `semantic_phi` observation for those
  cases
- 2026-04-14 supervisor then ruled out the simplest remaining select-route
  inventory as the next honest code-moving packet too:
  both `backend_codegen_route_riscv64_single_param_select_eq_asm_unsupported`
  and `backend_codegen_route_riscv64_two_param_select_eq_asm_unsupported`
  already emit `bir.select`, so the current backend-route test inventory no
  longer exposes another backlog-item-1 merge gap without a new observation
  path
- ref-based route confirmation recorded:
  `ref/claudes-c-compiler/src/backend/generation.rs` handles CFG semantics in
  the shared backend pipeline before later target work, while
  `ref/claudes-c-compiler/src/backend/stack_layout/mod.rs` and
  `stack_layout/slot_assignment.rs` treat multi-definition values as a
  downstream consequence of phi elimination rather than as a call-lane concern
- next packet therefore returns to backlog item 1:
  add one minimal semantic-BIR observation/harness surface that can show both
  canonical `select` and explicit `phi` merge semantics before prepare, with
  params/calls kept as sentinels only until merge semantics move forward again
  in code
