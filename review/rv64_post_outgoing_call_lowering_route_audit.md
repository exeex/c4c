# RV64 Post-Outgoing-Call-Lowering Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `b7064c29`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to outgoing-call lowering family`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan and is the
  commit that rewrote canonical state onto the current broader
  `calls.cpp` outgoing-call lowering family
- commits since base: `2`

## Scope Reviewed

- `plan.md`
- `todo.md`
- `review/rv64_post_tls_globals_broader_route_audit.md`
- `src/backend/riscv/codegen/calls.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`

## Findings

- Medium: the implementation diff remains aligned with the active route. The
  shared RV64 header now exposes the routed outgoing-call trio plus the minimum
  float-class enum surface
  ([`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:123),
  [`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:477)),
  and `calls.cpp` lands the selected owner bodies for F128 pre-convert,
  stack-argument lowering, and register-argument staging
  ([`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:73),
  [`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:109),
  [`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:151)).
- Medium: the proof lane matches the route and is no longer just declaration
  coverage. The test suite now checks the shared outgoing-call declarations and
  two bounded text proofs covering scalar register-plus-stack staging and wide
  `I128`/`F128` staging, and both tests are wired into the focused backend test
  runner
  ([`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:570),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1405),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1463),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:8214)).
- Medium: this route now looks complete at the current packet granularity. The
  translated Rust inventory for the chosen family is exactly the landed trio
  before the already-landed call instruction / cleanup / result-store entries
  ([`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:29),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:55),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:185)),
  and the live C++ owner now explicitly parks only broader aggregate/struct
  lowering beyond the scalar and wide-scalar staging slices
  ([`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:296)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Treat the current outgoing-call lowering family as complete at the active
route level. Rewrite `plan.md` / `todo.md` before sending more executor work so
canonical state no longer presents this calls-family lane as the current active
item.

- do not send another direct packet against the same scalar / wide-scalar
  outgoing-call staging trio
- choose the next lifecycle route from the remaining broader visible work
  instead of letting execution continue under stale call-lowering wording
- keep parked broader families parked until canonical state explicitly selects
  one of them

## Rationale

`scripts/plan_change_gap.sh` reports `last_plan_change_sha=b7064c29` and
`last_plan_change_gap=2`, and the two commits since that checkpoint stayed
inside the exact route the lifecycle commit selected. There is no sign of
scope drift into memory, globals, or comparison follow-on work.

At the same time, there is no equally narrow continuation left on the current
route. The active family named in `plan.md` / `todo.md` was the outgoing-call
lowering trio, and those owner bodies plus their focused proof surface are now
landed. Any next step in `calls.cpp` would widen into the broader
aggregate/struct/variadic surface that the current route explicitly parked
rather than continue the same packet sequence.
