# RV64 Post-TLS-Globals Broader Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `2a9be6a0`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane
  pending post-tls-globals route`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan and already
  records that commit `85113707` completed the bounded TLS globals seam while
  intentionally pausing the lane for a fresh broader-route decision
- commits since base: `0`

## Scope Reviewed

- `plan.md`
- `todo.md`
- `review/rv64_post_tls_globals_route_audit.md`
- `src/backend/riscv/codegen/calls.cpp`
- `src/backend/riscv/codegen/globals.cpp`
- `src/backend/riscv/codegen/memory.cpp`
- `src/backend/riscv/codegen/comparison.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `src/backend/x86/codegen/calls.cpp`
- `src/backend/x86/codegen/x86_codegen.hpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs`

## Findings

- Medium: the branch is aligned with the active runbook and is still sitting on
  the intended blocked checkpoint. `scripts/plan_change_gap.sh` reports
  `last_plan_change_sha=2a9be6a0` and `last_plan_change_gap=0`, so there is no
  post-checkpoint implementation drift to absorb before choosing the next route.
- Medium: `globals.cpp` is now exhausted as a direct owner lane. The live RV64
  owner already makes `emit_global_addr_impl(...)`,
  `emit_label_addr_impl(...)`, and `emit_tls_global_addr_impl(...)` real
  ([`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:48),
  [`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:57),
  [`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:62)),
  which matches the full translated Rust inventory
  ([`ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs:6)).
- Medium: `memory.cpp` is no longer the smallest truthful next route. The live
  C++ owner already carries default and const-offset load/store lowering
  through the shared `SlotAddr` surface, including direct, indirect, and
  over-aligned paths
  ([`src/backend/riscv/codegen/memory.cpp`](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:59),
  [`src/backend/riscv/codegen/memory.cpp`](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:77),
  [`src/backend/riscv/codegen/memory.cpp`](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:112)).
  Reopening memory from this checkpoint would be a broader family reset into
  typed-indirect / broader GEP / shared-state follow-on work, not the nearest
  remaining seam.
- Medium: `comparison.cpp` is also exhausted for the currently chosen route.
  The live owner now carries integer compare, float compare, fused
  compare-and-branch, and select
  ([`src/backend/riscv/codegen/comparison.cpp`](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:132),
  [`src/backend/riscv/codegen/comparison.cpp`](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:191),
  [`src/backend/riscv/codegen/comparison.cpp`](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:210)),
  so any follow-on there would widen beyond the now-landed
  fused-branch/select packet rather than continue an adjacent bounded seam.
- Medium: `calls.cpp` now exposes the narrowest honest broader route. The live
  RV64 owner explicitly marks only three remaining translated owner entries:
  `emit_call_f128_pre_convert_impl(...)`,
  `emit_call_stack_args_impl(...)`, and
  `emit_call_reg_args_impl(...)`
  ([`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:121)),
  and the translated Rust source shows that those three methods form the
  remaining outgoing-call lowering family rather than unrelated parked work
  ([`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:29),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:55),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:185)).
- Medium: this next route is broader, but it is still grounded in an existing
  transitional shape instead of requiring backend-wide invention. The x86
  public surface already exposes the corresponding stack-arg and register-arg
  lowering entry points
  ([`src/backend/x86/codegen/x86_codegen.hpp`](/workspaces/c4c/src/backend/x86/codegen/x86_codegen.hpp:527),
  [`src/backend/x86/codegen/x86_codegen.hpp`](/workspaces/c4c/src/backend/x86/codegen/x86_codegen.hpp:530),
  [`src/backend/x86/codegen/x86_codegen.hpp`](/workspaces/c4c/src/backend/x86/codegen/x86_codegen.hpp:536)),
  and the x86 owner shows the same integration stage pattern of stack
  preparation plus argument-register staging ahead of call emission
  ([`src/backend/x86/codegen/calls.cpp`](/workspaces/c4c/src/backend/x86/codegen/calls.cpp:35),
  [`src/backend/x86/codegen/calls.cpp`](/workspaces/c4c/src/backend/x86/codegen/calls.cpp:57)).
- Medium: the proof lane will need to widen with the route. Current RV64 calls
  coverage only proves ABI config, instruction/cleanup, and result-store
  surfaces
  ([`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:520),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:547),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:559));
  there is no focused declaration/text proof yet for the outgoing-call prep and
  arg-staging family selected here.

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` so canonical state selects this broader next
route:

- route the next execution stage to the translated `src/backend/riscv/codegen/calls.cpp`
  outgoing-call lowering family centered on
  `emit_call_f128_pre_convert_impl(...)`,
  `emit_call_stack_args_impl(...)`, and
  `emit_call_reg_args_impl(...)`
- keep the shared RV64 delta minimal:
  add only the helper/header/proof surface needed to make that outgoing-call
  prep and argument-staging family truthful on the already-landed RV64 call
  ABI / instruction / cleanup foundation
- keep these families parked:
  broader `memory.cpp` typed-indirect and broader GEP owner follow-on work,
  any broader comparison follow-on beyond the now-landed
  `emit_fused_cmp_branch_impl(...)` / `emit_select_impl(...)` route, and any
  broader call-result expansion beyond the already-parked I128 / F128 result
  paths

## Rationale

The blocked post-TLS-globals checkpoint was correct: the lane needed to stop
once the last direct globals owner entry landed. From that checkpoint, the
next truthful step is not to reopen exhausted globals or to reset into a wider
memory/comparison family. The smallest remaining translated owner inventory now
visible on the live RV64 surface is the outgoing-call lowering trio in
`calls.cpp`.

This is a broader route, not another helper-only packet. It needs canonical
state to acknowledge that the lane is moving from single-entry owner seams to
a bounded multi-method call-lowering family, with a matching proof surface. But
it is still the narrowest honest continuation because the surrounding backend
already carries the call ABI contract, direct/indirect call emission, cleanup,
and x86-shaped transitional entry points that this RV64 route can mirror.
