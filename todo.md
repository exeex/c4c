# Backend Reboot From BIR Spine

Status: Active
Source Idea: ideas/open/46_backend_reboot_bir_spine.md
Source Plan: plan.md

## Current Active Item

- active route:
  semantic BIR remains the truth surface, `prepare` owns target legality, and
  target backends should eventually ingest prepared BIR only
- review checkpoint:
  the 2026-04-14 route review judged the post-`b21fdb42` execution path as
  testcase-surface drift rather than backlog-item-1 capability progress
- packet evidence checkpoint:
  focused proof on 2026-04-14 showed that still-generic merge/select route
  stems such as `single_param_select_eq` and
  `two_param_select_eq_split_predecessor_add_phi_post_add_sub_add` already
  lower to semantic BIR today, while `branch_if_eq` remains green, so more
  one-stem promotions here would be route-surface churn rather than semantic
  CFG/`phi` work
- signature proof checkpoint:
  the first backlog-item-2 arithmetic route candidates are not a trustworthy
  lowering-owned proving surface:
  `two_param_add_sub_chain`, `two_param_i64_add_sub_chain`, and
  `two_param_staged_affine` already pass as semantic-BIR route coverage;
  `two_param_add` emits native riscv64 asm rather than printed BIR, which does
  not by itself prove a missing semantic-lowering capability in the owned
  files; and the current `i8`/`u8` route expectations demand narrower
  pre-promotion arithmetic than the frontend's current integer-promotion
  semantics
- current capability family:
  backlog item 2, harden params and function signatures
- current packet focus:
  the `two_arg_*` helper route surface is not an honest proving anchor under
  the current backend emission contract:
  once semantic BIR lowering succeeds there, `src/backend/backend.cpp`
  prepares BIR and emits native target asm, so the next packet must pivot to
  signature-sensitive backend behavior rather than printed-BIR expectations on
  those helpers
- queued follow-on surface:
  use `param_slot`, `param_member_array`, and
  `nested_param_member_array` as the next honest backlog-item-2 proving
  surface, while keeping the existing `two_arg_*` helper family only as
  runtime/regression sentinels
- packet rule:
  do not accept a packet whose main effect is promoting one more named
  arithmetic route stem, rewriting expectations to match current native asm, or
  suppressing frontend integer-promotion semantics; the next packet must show
  real signature-lowering capability growth on the parameter/aggregate surface
  or explicitly add the minimal harness observation needed to see that growth

## Immediate Target

- treat backlog item 1 as exhausted for the current proving surface unless a
  new semantic merge failure appears in lowering code
- treat the `two_arg_*` helper route cases as sentinels only:
  they may remain in proof as regression checks, but they are no longer the
  packet's primary evidence surface because successful lowering there still
  prints supported target asm
- broaden parsed parameter and return handling in semantic BIR through
  `param_slot`, `param_member_array`, and
  `nested_param_member_array` before touching broader call provenance or
  later-lane runtime surfaces
- if direct observation help is needed, authorize the smallest harness packet
  that can distinguish semantic-BIR signature success without regressing
  prepared-BIR emission, instead of retargeting helper-route expectations
- keep existing helper and indirect-call cases as non-regression sentinels only
- do not treat native asm output on a trivial helper as proof that semantic BIR
  lowering failed
- do not "fix" the current `i8`/`u8` route mismatches by suppressing the
  frontend's promoted `sext`/`zext` + `i32` + `trunc` semantics
- keep merge and indirect-call behavior as sentinels only; do not reopen route
  promotion, indirect-call provenance widening, globals, or runtime intrinsics
  in this packet

## Done Condition For The Active Packet

- at least one of `param_slot`, `param_member_array`, or
  `nested_param_member_array` demonstrates real backlog-item-2
  signature-lowering progress in the targets named by `plan.md`, either by
  honest backend behavior or by an explicitly authorized harness observation
- existing `two_arg_*` helper forms remain consistent as regression sentinels
  rather than requiring named-case branches or output-mode rewrites
- `branch_if_eq.c` still lowers cleanly and the already-proven merge route
  surface does not regress
- existing call-lane sentinels stay green
- no direct route, rendered-text matcher, or tiny named-case special path is
  introduced

## Proof Routing Note

- supervisor should choose a narrow build-plus-test command anchored on
  `param_slot`, `param_member_array`, and `nested_param_member_array`, with
  one existing `two_arg_*` helper case, `branch_if_eq`, and one indirect-call
  sentinel as non-regression checks
- if that subset still cannot directly observe signature-lowering success, the
  next packet may be a minimal harness-observation slice rather than a
  lowering-only slice
- do not reuse the exhausted arithmetic-route regexes or the helper-only route
  regex as the primary proving subset for the next executor packet

## Latest Packet Progress

- route switch recorded:
  focused proof showed the current backlog-item-1 packet was exhausted as a
  code task rather than blocked by missing merge semantics:
  `ctest --test-dir build -j --output-on-failure -R '^backend_codegen_route_riscv64_single_param_select_eq_asm_unsupported$'`
  failed with `[BACKEND_ASM_UNSUPPORTED_EXPECTED_FAIL]` because the case
  already lowers to semantic BIR as
  `%t8 = bir.select eq i32 %p.x, 7, i32 11, 4`
  `ctest --test-dir build -j --output-on-failure -R '^backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_add_phi_post_add_sub_add_asm_unsupported$'`
  likewise failed because the richer split-predecessor post-chain case already
  lowers through phi-slot load/store plus shared post-merge arithmetic
  `ctest --test-dir build -j --output-on-failure -R '^backend_codegen_route_riscv64_branch_if_eq_defaults_to_bir$'`
  still passed as the standing scalar-control-flow sentinel
  backlog item 1 therefore should not claim more progress via named route-stem
  promotion; the active packet now advances to backlog item 2 so the next work
  must be real signature-lowering capability growth
- signature proving-surface repair recorded:
  focused proof on the first backlog-item-2 arithmetic route candidates showed
  that they do not currently justify a code packet in the owned files:
  `ctest --test-dir build -j --output-on-failure -R '^backend_codegen_route_riscv64_two_param_add_sub_chain_defaults_to_bir$'`
  passed
  `ctest --test-dir build -j --output-on-failure -R '^backend_codegen_route_riscv64_two_param_i64_add_sub_chain_defaults_to_bir$'`
  passed
  `ctest --test-dir build -j --output-on-failure -R '^backend_codegen_route_riscv64_two_param_staged_affine_defaults_to_bir$'`
  passed
  `ctest --test-dir build -j --output-on-failure -R '^backend_codegen_route_riscv64_two_param_add_defaults_to_bir$'`
  failed by emitting native riscv64 asm (`add a0, a0, a1`) instead of printed
  BIR, which does not by itself prove a semantic-lowering gap in
  `lir_to_bir_module.cpp`
  `ctest --test-dir build -j --output-on-failure -R '^backend_codegen_route_riscv64_two_param_i8_add_sub_chain_defaults_to_bir$|^backend_codegen_route_riscv64_two_param_u8_add_defaults_to_bir$'`
  failed because the emitted BIR already reflects current frontend
  integer-promotion semantics through `sext`/`zext`, `i32` arithmetic, and
  `trunc`, so forcing those tests green here would amount to semantics-shaped
  overfit rather than honest signature-lowering progress
  backlog item 2 stays active, but the next executor packet must start from a
  different proving surface that demonstrates a real signature-lowering gap in
  the owned files
- signature route-suite exhaustion recorded:
  a follow-up scan of the current signature-adjacent route suite did not find
  another trustworthy backlog-item-2 packet in the existing tests:
  `ctest --test-dir build --output-on-failure -R '^backend_codegen_route_riscv64_two_param_add_defaults_to_bir$'`
  still failed only because the trivial two-parameter helper emitted native
  riscv64 asm (`add a0, a0, a1`) instead of printed BIR
  `ctest --test-dir build --output-on-failure -R '^backend_codegen_route_riscv64_two_param_add_sub_chain_defaults_to_bir$'`
  passed, confirming the non-trivial two-parameter `i32` signature route is
  already covered
  `ctest --test-dir build --output-on-failure -R '^backend_codegen_route_riscv64_two_param_i8_add_sub_chain_defaults_to_bir$|^backend_codegen_route_riscv64_two_param_u8_add_defaults_to_bir$'`
  failed only on frontend-promotion-shaped expectations:
  the emitted BIR widens to `i32`, performs the arithmetic there, and truncates
  back to `i8` as expected from current signed/unsigned promotion semantics
  no remaining existing route test now isolates a real parsed-param or
  return-lowering defect in `lir_to_bir_module.cpp` / `call_decode.cpp`, so
  the next autonomous step should be a lifecycle route checkpoint rather than a
  code packet guessed from weak evidence
- route reset recorded:
  the reviewer found that commits `3edfbf03..69b77578` changed only
  `tests/c/internal/InternalTests.cmake` plus `todo.md`, while backlog item 1
  promises semantic CFG/`phi` work in lowering/BIR files; future execution
  must return to code-changing merge semantics before more route-surface
  promotion is accepted
- lifecycle route checkpoint recorded:
  the runbook now narrows backlog item 2 onto the existing
  `backend_case` parameter surface instead of the exhausted
  `backend_route_case/two_param_*` arithmetic stems:
  direct scalar helper/call lowering through `two_arg_helper`,
  `two_arg_local_arg`, `two_arg_second_local_arg`, and
  `two_arg_both_local_arg` is the next executor packet,
  while `param_slot`, `param_member_array`, and
  `nested_param_member_array` are queued as the follow-on by-value aggregate
  parameter lane
- helper proving-surface invalidation recorded:
  the supervisor captured `test_before.log` with the helper-focused subset
  `cmake --build --preset default > test_before.log 2>&1 &&
   ctest --test-dir build -j --output-on-failure -R
   '^(backend_codegen_route_riscv64_two_arg_helper_defaults_to_asm|
      backend_codegen_route_riscv64_two_arg_local_arg_defaults_to_asm|
      backend_codegen_route_riscv64_two_arg_second_local_arg_defaults_to_asm|
      backend_codegen_route_riscv64_two_arg_both_local_arg_defaults_to_asm|
      backend_codegen_route_riscv64_branch_if_eq_defaults_to_bir|
      backend_codegen_route_riscv64_indirect_select_callee_call_defaults_to_bir)$'
   >> test_before.log 2>&1`
  and that subset passed unchanged, confirming only that the helper surface
  still emits supported riscv64 asm
  executor inspection then showed that `src/backend/backend.cpp` routes
  successful lowering through prepared BIR and still emits target asm on that
  path, so converting `two_arg_*` helper route tests into printed-BIR
  expectations would require either regressing supported emission or widening
  into harness work
  backlog item 2 therefore needs a route repair:
  `param_slot`, `param_member_array`, and `nested_param_member_array`
  become the next honest proving surface, while `two_arg_*` helper cases stay
  as regression sentinels only
