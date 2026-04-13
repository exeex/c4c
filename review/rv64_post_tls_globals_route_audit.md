# RV64 Post-TLS-Globals Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `be3b26fc`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane
  to tls globals seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan, and the
  only execution commit since then is the routed TLS globals packet at `HEAD`
- commits since base: `1`
  (`85113707 rv64: wire translated tls globals seam`)

## Scope Reviewed

- `src/backend/riscv/codegen/globals.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs`

## Validation Checked

- `cmake --build build --target backend_shared_util_tests`
- `./build/backend_shared_util_tests riscv_codegen_header_exports_translated_tls_global_addr_surface`
- `./build/backend_shared_util_tests riscv_translated_globals_tls_global_addr_emits_bounded_text`

Current log paths:

- `rv64_tls_globals_build.log`
- `rv64_tls_globals_header_test.log`
- `rv64_tls_globals_text_test.log`

## Findings

- Medium: the landed packet cleanly matches the active route. The live RV64
  globals owner now exposes `emit_tls_global_addr_impl(...)` through the shared
  header surface and lowers the translated Local-Exec `%tprel_*` path exactly
  through the routed owner entry
  ([`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:62),
  [`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:482),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs:27)).
- Medium: proof for the routed seam stays appropriately focused. The shared
  util coverage checks declaration reachability plus bounded emitted text for
  the Local-Exec TLS address materialization and destination-store paths
  without claiming broader call-lowering, memory, or runtime coverage
  ([`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:588),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1347)).
- Medium: canonical lifecycle state is stale again. `plan.md` and `todo.md`
  still present the TLS globals packet as the active execution slice even
  though commit `85113707` already landed it, so more execution would stack on
  top of an out-of-date checkpoint.
- Medium: there is no equally narrow direct follow-on left on the current
  globals lane. In the translated Rust owner, the visible globals inventory is
  exactly `emit_global_addr_impl(...)`, `emit_label_addr_impl(...)`, and
  `emit_tls_global_addr_impl(...)`, and all three are now real in the C++ tree
  ([`ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs:7),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs:20),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs:27)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks commit `85113707` as the landed bounded translated RV64 TLS globals
  seam with the focused proof lane above
- records that `emit_tls_global_addr_impl(...)` is now truthful through the
  shared RV64 globals surface
- blocks the lane pending a fresh broader-route decision instead of issuing
  another executor packet immediately, because the remaining visible follow-on
  work now widens beyond the exhausted globals owner lane

Keep these families parked:

- broader `calls.cpp` outgoing-call lowering and I128 / F128 result-store
  follow-on work
- broader `memory.cpp` typed-indirect and broader GEP owner work
- any broader comparison follow-on beyond the now-landed
  `emit_fused_cmp_branch_impl(...)` / `emit_select_impl(...)` route

## Rationale

The only execution commit since `be3b26fc` does exactly what that lifecycle
checkpoint asked for: it lands the bounded translated TLS globals seam,
exposes the minimum shared header surface, and proves the route with the named
focused checks. This is a coherent acceptance boundary, not drift.

The next honest action is not to guess another executor packet off stale
state. The visible translated RV64 globals owner is exhausted at this seam, so
the lane should return to a blocked broader-route checkpoint rather than
pretending there is another adjacent one-entry follow-on inside `globals.cpp`.
