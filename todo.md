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
- current capability family:
  backlog item 2, harden params and function signatures
- current packet shape:
  a code-changing semantic-signature packet against
  `src/backend/lowering/lir_to_bir_module.cpp` and
  `src/backend/lowering/call_decode.cpp`
- packet rule:
  do not accept a packet whose main effect is promoting one more named
  riscv64 route stem from `asm_unsupported` to `defaults_to_bir`; the next
  packet must show real signature-lowering capability growth

## Immediate Target

- treat backlog item 1 as exhausted for the current proving surface unless a
  new semantic merge failure appears in lowering code
- broaden parsed parameter and return handling in semantic BIR
- support wider multi-parameter and mixed simple-signature lowering without
  drifting into target ABI legalization
- keep merge and call-lane behavior as sentinels only; do not reopen route
  promotion or broader call provenance work in this packet
- choose proof from backend cases that expose signature-lowering limits, not
  from mislabeled merge-route stems that already lower to BIR

## Done Condition For The Active Packet

- semantic-lowering code changes land in the backlog-item-2 targets named by
  `plan.md`
- at least one broader multi-parameter or mixed simple-signature function
  lowers through semantic BIR because of that capability change
- `branch_if_eq.c` still lowers cleanly and the already-proven merge route
  surface does not regress
- existing call-lane sentinels stay green
- no direct route, rendered-text matcher, or tiny named-case special path is
  introduced

## Proof Command

- `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

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
- route reset recorded:
  the reviewer found that commits `3edfbf03..69b77578` changed only
  `tests/c/internal/InternalTests.cmake` plus `todo.md`, while backlog item 1
  promises semantic CFG/`phi` work in lowering/BIR files; future execution
  must return to code-changing merge semantics before more route-surface
  promotion is accepted
