# RV64 Post-Comparison-Control-Flow Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `5ca33ddf`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane
  to comparison control-flow seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan, and the
  only execution commit since then is the routed comparison-control-flow
  packet at `HEAD`
- commits since base: `1`
  (`c1d152ec rv64: wire translated comparison control-flow seam`)

## Scope Reviewed

- `plan.md`
- `todo.md`
- `src/backend/riscv/codegen/comparison.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`

## Validation Checked

- `cmake --build build --target backend_shared_util_tests`
- `./build/backend_shared_util_tests riscv_codegen_header_exports_translated_comparison_control_flow_surface`
- `./build/backend_shared_util_tests riscv_translated_comparison_control_flow_emits_bounded_text`

Current log paths:

- `rv64_cmp_control_flow_build.log`
- `rv64_cmp_control_flow_header_test.log`
- `rv64_cmp_control_flow_text_test.log`

## Findings

- Medium: the landed packet cleanly matches the active route. The live RV64
  comparison owner now exposes the routed
  `emit_fused_cmp_branch_impl(...)` / `emit_select_impl(...)` entries through
  the shared header surface and wires them onto the already-landed compare
  operand loader without widening into parked owner families
  ([`src/backend/riscv/codegen/comparison.cpp`](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:191),
  [`src/backend/riscv/codegen/comparison.cpp`](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:210),
  [`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:493)).
- Medium: proof for the routed seam stays appropriately focused. The shared
  util coverage checks declaration reachability plus bounded emitted text for
  the inverted-branch lowering and select store/reuse paths without claiming
  broader control-flow, call-lowering, or memory-owner coverage
  ([`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:610),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1600)).
- Medium: canonical lifecycle state is stale again. `plan.md` and `todo.md`
  still present the broader comparison-control-flow packet as the next active
  execution slice even though commit `c1d152ec` already landed it, so more
  execution would stack on top of an out-of-date checkpoint
  ([`plan.md`](/workspaces/c4c/plan.md:54),
  [`plan.md`](/workspaces/c4c/plan.md:60),
  [`todo.md`](/workspaces/c4c/todo.md:9)).
- Medium: there is no equally narrow direct follow-on left on the current RV64
  comparison lane. In the translated Rust owner, the remaining comparison
  inventory after integer compare is exactly fused compare-and-branch, select,
  and the already-landed F128 compare hook, so the comparison owner is now
  consumed at this route boundary rather than leaving another helper-sized
  packet in the same family
  ([`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:75),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:111),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:140)).
- Medium: the remaining visible follow-on work is still the already-parked
  broader families rather than another immediate executor packet. The current
  lifecycle state explicitly keeps broader calls, TLS globals, and
  typed-indirect / broader GEP memory work parked, and nothing in the just-
  landed diff narrows one of those families enough to justify choosing it
  without a fresh broader-route decision
  ([`plan.md`](/workspaces/c4c/plan.md:67),
  [`todo.md`](/workspaces/c4c/todo.md:36)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks commit `c1d152ec` as the landed broader translated
  `comparison.cpp` control-flow seam with the focused proof lane above
- records that `emit_fused_cmp_branch_impl(...)` and `emit_select_impl(...)`
  are now truthful through the shared RV64 comparison surface
- blocks the lane pending a fresh broader-route decision instead of issuing
  another executor packet immediately, because the remaining visible follow-on
  work now widens into already-parked broader calls, TLS globals, or
  typed-indirect / broader GEP memory work rather than another equally narrow
  comparison-owner slice

Keep these families parked:

- broader `calls.cpp` outgoing-call lowering and I128 / F128 result-store
  follow-on work
- `globals.cpp::emit_tls_global_addr_impl(...)`
- broader `memory.cpp` typed-indirect and broader GEP owner work
- any broader comparison expansion beyond the now-landed
  `emit_fused_cmp_branch_impl(...)` / `emit_select_impl(...)` route

## Rationale

The only execution commit since `5ca33ddf` does exactly what that lifecycle
checkpoint asked for: it lands the broader translated comparison-control-flow
seam, exposes the minimum shared header surface, and proves the route with the
named focused checks. This is a coherent acceptance boundary, not drift.

The next honest action is not to guess another executor packet off stale
state. The visible translated comparison owner is exhausted at this seam, and
the remaining families called out by current lifecycle state are still broader
parked routes rather than another adjacent helper-sized follow-on. That makes
this another lifecycle rewrite point, with a blocked checkpoint, instead of a
direct continuation into implementation.
