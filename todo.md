# Backend Reboot From BIR Spine

Status: Active
Source Idea: ideas/open/46_backend_reboot_bir_spine.md
Source Plan: plan.md

## Lifecycle Repair Decision

- 2026-04-14 plan-owner checkpoint:
  this remains the same active source idea and the same active runbook, so no
  `plan.md` or source-idea rewrite is justified yet
- execution state:
  blocked for routine executor dispatch until one fresh backlog-item-1
  code-moving merge seam is identified outside the exhausted scalar
  select/phi inventory already recorded below
- next lifecycle expectation:
  supervisor may do broader validation or route scouting, but should not send
  another executor packet from the stale `asm_unsupported` select/phi stems or
  from observation-only harness churn; if no honest merge-semantics packet can
  be named after that checkpoint, return for another lifecycle repair instead
  of pretending the current inventory is still exposing new capability work

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
  return to code-moving backlog-item-1 work rather than more harness churn;
  supervisor follow-up has now confirmed the remaining rv64 `u8`
  select/phi `asm_unsupported` inventory is stale too, so the current
  `backend_codegen_route` inventory no longer exposes another honest
  code-moving merge packet by itself;
  one route-unification follow-up packet has now landed too:
  canonical diamond-select lowering no longer enters BIR through a separate
  top-level select-only dispatch, because `lower_module` now routes active
  backend functions through `lower_branch_family_function`, and that shared
  branch/phi lane directly owns the canonical select lowering helper
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
  remaining backlog-item-1 merge gap cleanly;
  supervisor then confirmed the same exhaustion for the remaining rv64 `u8`
  select/phi inventory:
  both `backend_codegen_route_riscv64_single_param_u8_select_eq_asm_unsupported`
  and
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_add_phi_post_add_sub_asm_unsupported`
  now fail because the backend already emits prepared BIR plus
  `semantic_phi` observation for those stems too;
  supervisor then spot-checked the nearest "fresh-looking" non-stale merge
  surfaces and found that they are already green on the truthful prepared-BIR
  route too:
  `three_way_phi_merge_post_add_sub.c` now lowers to nested prepared
  `bir.select`, and a nested indirect-callee ternary probe also lowers to
  prepared BIR plus `bir.call` without private merge reconstruction, so those
  surfaces are route evidence only, not the next honest code-moving packet
- packet rule:
  do not accept more `todo` / `InternalTests.cmake` churn as proxy progress;
  stale unsupported-test promotion for the split-predecessor
  `*_post_add_sub_add` stems plus the newly confirmed
  `two_param_select_eq_predecessor_add_post_add` and
  `two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub`
  stems is harness cleanup only, not backlog-item-1 progress;
  the remaining rv64 `u8` `single_param_select_eq` and
  `two_param_u8_select_eq_split_predecessor_add_phi_post_add_sub`
  `asm_unsupported` stems are now the same kind of stale harness debt, not a
  fresh merge target;
  the authorized one-off observation/harness slice is already spent, so the
  next accepted packet must include a real backlog-item-1 code move and may
  use the observation surface only as proof support, not as a substitute for
  new lowering/backend capability work

## Immediate Target

- the active plan remains on backlog item 1, generalize CFG merge and `phi`,
  but the currently sampled `backend_codegen_route` inventory is exhausted as
  a source of honest next packets
- lifecycle repair outcome:
  do not dispatch another executor packet from the current scalar
  select/phi/callee inventory;
  the next action is to identify one fresh backlog-item-1 code-moving merge
  target outside that exhausted surface, or else perform another route
  checkpoint instead of pretending the nearby prepared-BIR cases are still
  exposing a missing capability
- 2026-04-14 supervisor repair probe:
  checked-in `three_way_phi_merge_post_add_sub.c` still lowers cleanly on the
  default prepared route to nested `bir.select`, and a fresh throwaway
  three-predecessor local-join sample lowered through ordinary local-slot
  stores/loads rather than semantic `bir.phi`;
  current C backend-route coverage therefore still does not expose a fresh
  non-reducible/default-route phi seam honestly enough to justify another
  executor packet from backlog item 1 yet
- the next packet must return to one real code-moving backlog-item-1 merge
  target using the semantic-BIR observation surface only as proof support;
  do not spend another packet on route-test promotion or harness-only cleanup
- if the executor needs a new proving source to expose that code-moving target,
  it may add one minimal merge-semantic source in the same packet, but source
  or harness expansion alone is not accepted progress
- keep backlog item 2 params/signatures and backlog item 5 call lowering as
  sentinels only until backlog item 1 has another real code change
- treat the split-predecessor `*_post_add_sub_add` stems plus the newly stale
  `two_param_select_eq_predecessor_add_post_add` and
  `two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub`
  stems as harness debt, not as the next executor target
- treat the remaining rv64 `u8` `single_param_select_eq` and
  `two_param_u8_select_eq_split_predecessor_add_phi_post_add_sub`
  `asm_unsupported` stems as the same stale harness debt
- keep call lowering out of scope for the next code packet except as a
  consumer-side sentinel:
  no new call-specific merge reconstruction is allowed
- keep helper params, indirect-call provenance, globals, and runtime intrinsics
  out of scope for this repair
- treat `three_way_phi_merge_post_add_sub.c` and nested indirect-callee
  ternary probes as confirmation that the current route already handles those
  nearby prepared-BIR shapes; they are not the next executor target unless a
  new code change broadens semantics beyond the now-proven route
- for proof, anchor the harness on one canonical diamond-select case and one
  explicit merge case, then keep `branch_if_eq` and one existing call-lane
  sentinel as non-regression checks; do not use `two_arg_*` helper output mode
  as the primary evidence surface

## Done Condition For The Next Accepted Packet

- one real backlog-item-1 code move lands in lowering/BIR/prepare files rather
  than only in route-test or harness wiring
- the code move broadens honest merge semantics beyond the already exhausted
  prepared-BIR route inventory, with semantic-BIR observation used only where
  it directly proves that new capability
- `branch_if_eq.c` still lowers cleanly
- existing call-lane sentinels stay green without new call-specific merge logic
- no direct route, rendered-text matcher, or tiny named-case special path is
  introduced

## Proof Routing Note

- supervisor should choose a narrow build-plus-test command anchored on the
  new code-moving merge target, using the semantic-BIR observation surface
  only where it directly proves that target, plus `branch_if_eq` and one
  call-lane sentinel as non-regression checks
- do not reuse helper-only param surfaces or arithmetic-route regex churn as
  the primary proving subset for the next executor packet
- the one-off observation/harness authorization is already spent;
  after it landed, the route returns to code-moving backlog-item-1 work only
- stale unsupported split-predecessor tests may be cleaned up later, but that
  cleanup must follow a code-moving packet or be explicitly treated as harness
  debt rather than accepted capability progress
- because current `backend_codegen_route` tests observe prepared BIR, the next
  accepted packet may introduce a new proving source only when it is tied to a
  real merge-semantics code move rather than pretending the exhausted
  prepared-BIR route inventory still exposes a missing merge family cleanly
- if no fresh code-moving backlog-item-1 proving source can be named without
  returning to observation churn or stale inventory cleanup, treat that as a
  route-checkpoint condition and repair lifecycle state again before sending an
  executor
- 2026-04-14 supervisor route-checkpoint follow-up tightened that condition:
  broader backend logs are currently stable at `208 passed / 211 failed / 419
  total`, so acceptance confidence is no longer blocked on a fresh regression
  delta, but the nearby "fresh-looking" merge surfaces are exhausted too;
  `return_select_eq[_u8]` and `return_select_ne[_u8]` currently fail because
  the route emits truthful prepared BIR or native asm shape rather than the old
  literal-snippet expectations, and `three_way_phi_merge_post_add_sub.c`
  already lowers on the default prepared route to nested `bir.select` while the
  semantic observation route still exposes explicit `bir.phi`
- the remaining plausible backlog-item-1 seam after that check is no longer the
  stale select inventory itself but a future generalization of explicit phi
  handling beyond the currently proven binary/reducible shapes in
  `src/backend/prepare/legalize.cpp`; do not dispatch another executor packet
  until that seam is tied to one honest proving surface instead of another
  stale-harness promotion
- 2026-04-14 supervisor route-repair follow-up then tested whether a new
  three-predecessor C probe could expose that seam on the normal route and it
  could not:
  the throwaway sample lowered as mutable-local slot traffic instead of
  semantic `bir.phi`, so no truthful new proving surface was found from the
  current C route inventory; treat backlog-item-1 execution as blocked on the
  next route checkpoint rather than forcing another executor packet from this
  exhausted surface

## Latest Packet Progress

- 2026-04-14 executor landed one backlog-item-1 route-unification code move
  in `src/backend/lowering/lir_to_bir_module.cpp`:
  `lower_canonical_select_function` no longer reconstructs the
  `indirect_select_callee_call` merge/call lane privately;
  canonical-select fast-path lowering is now restricted back to direct-return
  diamonds, so the indirect-callee sentinel falls through to the shared
  branch/phi lowering path and the prepared-BIR route now observes the same
  temporary phi-slot materialization contract as other merge consumers
- 2026-04-14 exact delegated proof for that indirect-callee merge packet
  passed as
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_riscv64_(indirect_select_callee_call_defaults_to_bir|indirect_select_local_override_callee_call_defaults_to_bir|branch_if_eq_defaults_to_bir))$'`
  and wrote the result to `test_after.log`
- 2026-04-14 route coverage for that packet now keeps
  `backend_codegen_route_riscv64_indirect_select_callee_call_defaults_to_bir`
  on the truthful shared-merge prepared-BIR surface:
  the route no longer proves progress with `bir.select` plus a private call
  fast path, and instead observes the existing phi-slot prepare contract while
  `backend_codegen_route_riscv64_indirect_select_local_override_callee_call_defaults_to_bir`
  and `backend_codegen_route_riscv64_branch_if_eq_defaults_to_bir` stay green
- 2026-04-14 executor landed one backlog-item-1 route-unification code move
  in `src/backend/lowering/lir_to_bir_module.cpp`:
  `lower_module` now dispatches active backend functions directly through
  `lower_branch_family_function`, and that shared branch/phi lane now owns
  the canonical diamond-select lowering helper instead of relying on a
  separate top-level select-family fast path
- 2026-04-14 exact delegated proof for that route-unification packet passed as
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_riscv64_(single_param_select_eq_observes_semantic_bir|two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_observes_semantic_bir|branch_if_eq_defaults_to_bir|indirect_select_local_override_callee_call_defaults_to_bir))$'`
  and wrote the result to `test_after.log`
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
- 2026-04-14 supervisor then confirmed the same stale-harness outcome for the
  remaining rv64 `u8` merge inventory:
  both `backend_codegen_route_riscv64_single_param_u8_select_eq_asm_unsupported`
  and
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_add_phi_post_add_sub_asm_unsupported`
  currently fail because the backend already emits prepared BIR plus
  `semantic_phi` observation for those stems too
- ref-based route confirmation recorded:
  `ref/claudes-c-compiler/src/backend/generation.rs` handles CFG semantics in
  the shared backend pipeline before later target work, while
  `ref/claudes-c-compiler/src/backend/stack_layout/mod.rs` and
  `stack_layout/slot_assignment.rs` treat multi-definition values as a
  downstream consequence of phi elimination rather than as a call-lane concern
- next packet therefore still returns to backlog item 1, but only as one real
  code-moving merge packet:
  the semantic-BIR observation surface remains available as proof support,
  while params/calls stay as sentinels and the exhausted route inventory is no
  longer an accepted proxy for merge progress
- 2026-04-14 executor landed one backlog-item-1 canonical-diamond code move
  in `src/backend/lowering/lir_to_bir_module.cpp`:
  `lower_canonical_select_function` now accepts the narrow compare prelude and
  foldable arm-cast chain needed by `single_param_u8_select_eq.c`, so that
  case lowers through `bir.select` instead of falling back to the temporary
  phi-slot load/store path
- 2026-04-14 exact delegated proof for that canonical-`u8` select packet
  passed as
  `cmake --build --preset default && rm -f build/internal_backend_route/single_param_u8_select_eq_probe_riscv64.ll && ./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/c/internal/backend_route_case/single_param_u8_select_eq.c -o build/internal_backend_route/single_param_u8_select_eq_probe_riscv64.ll && rg -F "bir.select eq i32 %t0, 7, i8 11, 4" build/internal_backend_route/single_param_u8_select_eq_probe_riscv64.ll && ! rg -F "bir.store_local %t11.phi" build/internal_backend_route/single_param_u8_select_eq_probe_riscv64.ll && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_riscv64_(branch_if_eq_defaults_to_bir|indirect_select_local_override_callee_call_defaults_to_bir))$'`
  and wrote the result to `test_after.log`
- the remaining backlog-item-1 gap after this packet is broader than this one
  canonical-`u8` arm fold:
  stale `asm_unsupported` route entries remain harness debt only, while the
  next accepted packet still needs another real merge-semantics code move
  rather than unsupported-test promotion
- 2026-04-14 executor extended canonical-diamond select lowering to cover a
  short post-merge scalar use chain on the join block (for example, `add` and
  `trunc` before `ret`) while also allowing the select-arm chains to compute
  their incoming values via simple scalar ops instead of forcing the
  temporary phi-slot materialization path; this moved
  `two_param_select_eq_predecessor_add_post_add.c` and
  `two_param_u8_select_eq_predecessor_add_post_add.c` onto `bir.select` plus
  the post-merge `add` (and `trunc` for the `u8` case)
- 2026-04-14 exact delegated proof for that post-merge canonical-select packet
  ran as
  `cmake --build --preset default && rm -f build/internal_backend_route/two_param_select_eq_predecessor_add_post_add_probe_riscv64.ll build/internal_backend_route/two_param_u8_select_eq_predecessor_add_post_add_probe_riscv64.ll && ./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/c/internal/backend_route_case/two_param_select_eq_predecessor_add_post_add.c -o build/internal_backend_route/two_param_select_eq_predecessor_add_post_add_probe_riscv64.ll && rg -F "%t10 = bir.select eq i32 %p.x, %p.y, i32 %t8, %t9" build/internal_backend_route/two_param_select_eq_predecessor_add_post_add_probe_riscv64.ll && rg -F "%t11 = bir.add i32 %t10, 6" build/internal_backend_route/two_param_select_eq_predecessor_add_post_add_probe_riscv64.ll && ! rg -F "bir.store_local %t10.phi" build/internal_backend_route/two_param_select_eq_predecessor_add_post_add_probe_riscv64.ll && ./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/c/internal/backend_route_case/two_param_u8_select_eq_predecessor_add_post_add.c -o build/internal_backend_route/two_param_u8_select_eq_predecessor_add_post_add_probe_riscv64.ll && rg -F "%t14 = bir.select eq i32 %t0, %t1, i32 %t11, %t13" build/internal_backend_route/two_param_u8_select_eq_predecessor_add_post_add_probe_riscv64.ll && rg -F "%t15 = bir.add i32 %t14, 6" build/internal_backend_route/two_param_u8_select_eq_predecessor_add_post_add_probe_riscv64.ll && ! rg -F "bir.store_local %t14.phi" build/internal_backend_route/two_param_u8_select_eq_predecessor_add_post_add_probe_riscv64.ll && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_riscv64_(branch_if_eq_defaults_to_bir|indirect_select_local_override_callee_call_defaults_to_bir))$' > test_after.log 2>&1`
  and wrote the result to `test_after.log`; route harness coverage for this
  capability was captured by adding explicit `defaults_to_bir` route tests for
  the two predecessor-add stems and removing them from the stale
  `*_asm_unsupported` inventory
- 2026-04-14 executor harness repair for the canonical-diamond post-merge
  generalization:
  several split-predecessor `defaults_to_bir` route tests were still asserting
  the old phi-slot materialization (`bir.store_local *.phi`), but the new
  canonical lowering legitimately emits single-block `bir.select` plus the
  post-merge arithmetic chain; those route tests were updated to assert the
  `bir.select` shape and forbid `bir.store_local` instead
  the existing semantic-observation test names were also repointed onto one
  minimal three-way merge case so they continue to prove a real explicit-merge
  `bir.phi` surface (semantic stage) and its prepared `semantic_phi` trailer
  rather than observing a now-canonical diamond
- 2026-04-14 exact delegated proof for that harness-repair packet ran as
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_riscv64_(branch_if_eq_defaults_to_bir|indirect_select_local_override_callee_call_defaults_to_bir|two_param_select_eq_split_predecessor_add_phi_post_add_sub_defaults_to_bir|two_param_select_eq_split_predecessor_add_phi_post_add_sub_add_defaults_to_bir|two_param_select_eq_split_predecessor_deeper_affine_post_add_defaults_to_bir|two_param_select_eq_split_predecessor_deeper_affine_post_add_sub_defaults_to_bir|two_param_select_eq_split_predecessor_deeper_affine_post_add_sub_add_defaults_to_bir|two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_defaults_to_bir|two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_add_defaults_to_bir|two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_observes_semantic_bir|two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_observes_semantic_phi|two_param_select_eq_split_predecessor_mixed_affine_post_add_defaults_to_bir|two_param_select_eq_split_predecessor_mixed_affine_post_add_sub_defaults_to_bir|two_param_select_eq_split_predecessor_mixed_affine_post_add_sub_add_defaults_to_bir|two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_defaults_to_bir|two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_add_defaults_to_bir))$' > test_after.log 2>&1`
  and wrote the result to `test_after.log`
- 2026-04-14 executor folded the existing nested ternary / three-way merge
  prepared route in `src/backend/prepare/legalize.cpp` from a reducible
  top-of-block `bir.phi` tree into a nested `bir.select` chain while leaving
  semantic-stage lowering unchanged, so
  `three_way_phi_merge_post_add_sub.c` now emits prepared nested selects but
  still exposes explicit semantic `bir.phi` under
  `--backend-bir-stage semantic`
- 2026-04-14 owned route expectations were updated to the truthful post-change
  surfaces:
  the indirect selected-callee defaults-to-BIR tests now assert
  `bir.select`-based callee choice instead of temporary phi-slot
  materialization, and the
  `two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_observes_semantic_phi`
  sentinel now observes the semantic-stage `bir.phi` chain directly rather
  than the old prepared `semantic_phi` trailer
- 2026-04-14 exact delegated proof for the nested-three-way-merge packet ran
  as
  `cmake --build --preset default && ./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/c/internal/backend_route_case/three_way_phi_merge_post_add_sub.c -o build/internal_backend_route/three_way_phi_merge_post_add_sub_probe_riscv64.ll && rg -F "%t19 = bir.select eq i32 %p.x, %p.z, i32 %t17, %t18" build/internal_backend_route/three_way_phi_merge_post_add_sub_probe_riscv64.ll && rg -F "%t20 = bir.select eq i32 %p.x, %p.y, i32 %t8, %t19" build/internal_backend_route/three_way_phi_merge_post_add_sub_probe_riscv64.ll && rg -F "%t21 = bir.add i32 %t20, 6" build/internal_backend_route/three_way_phi_merge_post_add_sub_probe_riscv64.ll && ! rg -F "bir.store_local %t19.phi" build/internal_backend_route/three_way_phi_merge_post_add_sub_probe_riscv64.ll && ! rg -F "bir.store_local %t20.phi" build/internal_backend_route/three_way_phi_merge_post_add_sub_probe_riscv64.ll && ./build/c4cll --backend-bir-stage semantic --codegen asm --target riscv64-unknown-linux-gnu tests/c/internal/backend_route_case/three_way_phi_merge_post_add_sub.c -o build/internal_backend_route/three_way_phi_merge_post_add_sub_semantic_probe_riscv64.ll && rg -F "%t19 = bir.phi i32 [tern.then.end.13, %t17] [tern.else.end.15, %t18]" build/internal_backend_route/three_way_phi_merge_post_add_sub_semantic_probe_riscv64.ll && rg -F "%t20 = bir.phi i32 [tern.then.end.4, %t8] [tern.else.end.6, %t19]" build/internal_backend_route/three_way_phi_merge_post_add_sub_semantic_probe_riscv64.ll && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_riscv64_(branch_if_eq_defaults_to_bir|indirect_select_local_override_callee_call_defaults_to_bir|single_param_select_eq_observes_semantic_bir|two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_observes_semantic_phi))$' > test_after.log 2>&1`
  and writes the result to `test_after.log`
