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
- current capability family:
  backlog item 1, generalize CFG merge and `phi`
- current packet shape:
  a code-changing semantic-lowering packet against
  `src/backend/lowering/lir_to_bir_module.cpp` and, only if needed,
  `src/backend/bir.hpp`
- packet rule:
  do not accept a packet whose main effect is promoting one more named
  riscv64 route stem from `asm_unsupported` to `defaults_to_bir`

## Immediate Target

- widen CFG merge lowering by semantics, not by testcase name
- make non-diamond merge values survive ordinary block joins through shared
  lowering rules rather than per-stem route expectations
- keep call-lane behavior as a sentinel only; do not drift back into call
  packets
- if execution proves the capability is already present, stop and record the
  evidence instead of continuing one-stem-at-a-time route promotion

## Done Condition For The Active Packet

- semantic-lowering code changes land in the backlog-item-1 targets named by
  `plan.md`
- at least one richer non-diamond merge shape lowers through semantic BIR
  because of that capability change, not only because a route assertion moved
- `branch_if_eq.c` still lowers cleanly
- existing call-lane sentinels stay green
- no direct route, rendered-text matcher, or tiny named-case special path is
  introduced

## Proof Command

- `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

## Latest Packet Progress

- route reset recorded:
  the reviewer found that commits `3edfbf03..69b77578` changed only
  `tests/c/internal/InternalTests.cmake` plus `todo.md`, while backlog item 1
  promises semantic CFG/`phi` work in lowering/BIR files; future execution
  must return to code-changing merge semantics before more route-surface
  promotion is accepted
