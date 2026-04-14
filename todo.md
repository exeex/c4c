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
- current packet shape:
  the next code-changing semantic-signature packet should still target
  `src/backend/lowering/lir_to_bir_module.cpp` and
  `src/backend/lowering/call_decode.cpp`, but it must start from a proving
  surface that demonstrates a real signature-lowering gap in those owned files
  rather than from arithmetic route cases that are already covered,
  emitter-routed, or semantically mismatched
- route checkpoint needed:
  the current route suite no longer provides an honest backlog-item-2
  executor packet:
  the remaining visible failures in existing signature-adjacent route cases
  are either route-selection artifacts (`two_param_add` emitting native
  riscv64 asm for a trivial helper), frontend-promotion expectation mismatches
  (`i8`/`u8` arithmetic widening and truncation), or later-lane call/runtime
  behavior outside the current lowering-owned proving surface
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
- do not treat native asm output on an arithmetic route case as proof that
  semantic BIR lowering failed
- do not "fix" the current `i8`/`u8` route mismatches by suppressing the
  frontend's promoted `sext`/`zext` + `i32` + `trunc` semantics
- keep merge and call-lane behavior as sentinels only; do not reopen route
  promotion or broader call provenance work in this packet
- choose proof from backend cases that expose signature parsing or return/param
  lowering limits in semantic BIR itself, not from mislabeled merge-route
  stems, emitter-routed arithmetic cases, or tests whose expectation conflicts
  with current frontend integer-promotion meaning

## Done Condition For The Active Packet

- the next executor packet for backlog item 2 is chosen from a proving surface
  that is owned by semantic signature lowering in the backlog-item-2 targets
  named by `plan.md`
- at least one broader multi-parameter or mixed simple-signature function
  lowers through semantic BIR because of that capability change, not merely
  because a route test was retargeted or because native asm happened to print
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
