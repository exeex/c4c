# RV64 Post-Outgoing-Call Broader Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `0bdc1e9e`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane pending post-outgoing-call route`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan and already
  records that the scalar / wide-scalar outgoing-call staging route is
  complete while the lane waits for a fresh broader-route decision
- commits since base: `0`

## Scope Reviewed

- `plan.md`
- `todo.md`
- `review/rv64_post_outgoing_call_lowering_route_audit.md`
- `src/backend/riscv/codegen/calls.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `src/backend/riscv/codegen/variadic.cpp`
- `src/backend/x86/codegen/calls.cpp`
- `src/backend/x86/codegen/x86_codegen.hpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`

## Findings

- Medium: the blocked checkpoint is correct. `scripts/plan_change_gap.sh`
  reports `last_plan_change_sha=0bdc1e9e` and `last_plan_change_gap=0`, so
  there is no post-checkpoint implementation drift to absorb before choosing
  the next broader route.
- Medium: `calls.cpp` remains the narrowest honest broader family. The live C++
  owner now fully covers scalar, `I128`, and `F128` outgoing-call staging, but
  the translated Rust inventory still contains aggregate / struct stack and
  register lowering inside the same two owner entry points
  ([`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:63),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:147),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:309),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:359)).
- Medium: `memory.cpp` is not a smaller next step from this checkpoint. The
  active plan already treats typed-indirect / broader GEP memory work as a
  separate broader family, and nothing in the post-outgoing-call diff changes
  that judgment. Reopening memory here would be a route jump, not the nearest
  visible continuation.
- Medium: `comparison.cpp` is also still exhausted for this branch. The current
  plan already landed the fused-branch/select route, and there is no new diff
  or review evidence that makes comparison follow-on smaller than the remaining
  aggregate / struct calls inventory.
- Medium: the shared surface is already prepared for the next calls route. The
  `CallArgClass` variants and `RiscvFloatClass` plumbing needed by aggregate /
  struct call lowering are already present in the shared header
  ([`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:157),
  [`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:179),
  [`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:489)),
  and existing focused tests already exercise the ABI config and stack-space
  modeling for `StructSplitRegStack` and `StructByValStack`
  ([`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1220)).
- Medium: `variadic.cpp` should stay parked for now. The repo already treats it
  as a separate translated owner surface, and widening the next route into
  variadic state/save-area work would expand beyond the still-adjacent
  aggregate / struct staging inventory in `calls.cpp`
  ([`src/backend/riscv/codegen/variadic.cpp`](/workspaces/c4c/src/backend/riscv/codegen/variadic.cpp:1)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` so canonical state selects the broader
translated `calls.cpp` aggregate / struct outgoing-call lowering family as the
next truthful route.

- center the next lifecycle stage on the remaining aggregate / struct paths in
  `emit_call_stack_args_impl(...)` and `emit_call_reg_args_impl(...)`
- treat `StructByValStack`, `LargeStructStack`, `StructSplitRegStack`,
  `StructByValReg`, `StructSseReg`, `StructMixedIntSseReg`, and
  `StructMixedSseIntReg` as the visible translated family inventory
- add only the minimum helper/proof support needed to make those aggregate /
  struct staging paths truthful on the already-landed scalar / wide-scalar
  outgoing-call foundation
- keep broader memory follow-on, broader comparison follow-on, broader
  call-result expansion, and `variadic.cpp` parked

## Rationale

The current blocked checkpoint should not remain idle: the post-outgoing-call
review established that the scalar / wide-scalar route is complete, and the
remaining translated inventory that is still adjacent in the same owner file is
aggregate / struct staging. That is broader than the route that just landed,
but it is still a more honest continuation than jumping back into unrelated
memory or comparison families.

The x86 side also reinforces that call lowering stays cohesive across stack and
register staging families rather than splitting immediately into unrelated
owner files
([`src/backend/x86/codegen/calls.cpp`](/workspaces/c4c/src/backend/x86/codegen/calls.cpp:35),
[`src/backend/x86/codegen/calls.cpp`](/workspaces/c4c/src/backend/x86/codegen/calls.cpp:57)).
