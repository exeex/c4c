# RV64 Post-Float-Binop Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `70fdac12`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to float binop seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan, and the
  only commit since then is the landed implementation slice for that exact
  packet
- commits since base: `1`
  (`f4c66bfe riscv: wire translated float binop seam`)

## Scope Reviewed

- `src/backend/riscv/codegen/float_ops.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `CMakeLists.txt`
- `src/backend/riscv/codegen/comparison.cpp`
- `src/backend/riscv/codegen/variadic.cpp`
- `src/backend/riscv/codegen/f128.cpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/{float_ops,comparison,variadic}.rs`

## Validation Checked

- `cmake --build build --target backend_shared_util_tests c4cll`
- `./build/backend_shared_util_tests riscv_codegen_header_exports_translated_float_binop_surface`
- `./build/backend_shared_util_tests riscv_translated_float_binop_non_f128_emits_bounded_text`

All three commands passed at this checkpoint.

## Findings

- Medium: the landed float-binop packet is aligned with the active route and
  stays bounded. The shared RV64 surface gained only `FloatOp` plus the
  translated non-`F128` owner entry, `float_ops.cpp` entered the active build
  lists, and the focused tests prove the scalar F32/F64 text path without
  widening into broader floating-point work
  ([`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:50),
  [`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:308),
  [`src/backend/riscv/codegen/float_ops.cpp`](/workspaces/c4c/src/backend/riscv/codegen/float_ops.cpp:57),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:380),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:871)).
- Medium: the remaining visible float-side follow-on is not the next narrow
  packet. `emit_f128_neg_impl(...)` is still parked because the current RV64
  C++ surface only exposes `emit_f128_operand_to_a0_a1(...)`; the shared
  `emit_f128_neg_full(...)`/soft-float orchestration path is still absent, so
  continuing inside `float_ops.cpp` would widen the lane into broader F128
  owner work
  ([`src/backend/riscv/codegen/f128.cpp`](/workspaces/c4c/src/backend/riscv/codegen/f128.cpp:98),
  [`src/backend/riscv/codegen/float_ops.cpp`](/workspaces/c4c/src/backend/riscv/codegen/float_ops.cpp:62)).
- Medium: the narrowest practical follow-on is now a comparison packet, but
  only the non-`F128` float-compare owner entry. `comparison.cpp` already has
  the float predicate and operand-order helper tables, and
  `emit_float_cmp_impl(...)` can reuse the landed `operand_to_t0(...)`,
  `store_t0_to(...)`, and shared `IrType` surface after adding only a minimal
  shared `IrCmpOp` declaration. The remaining comparison bodies still imply
  wider shared state such as compare-operand loaders, fresh labels, and branch
  cache management, while `variadic.cpp` still depends on unshared
  va-list-state queries and pointer-location helpers
  ([`src/backend/riscv/codegen/comparison.cpp`](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:12),
  [`src/backend/riscv/codegen/comparison.cpp`](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:55),
  [`src/backend/riscv/codegen/comparison.cpp`](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:155),
  [`src/backend/riscv/codegen/variadic.cpp`](/workspaces/c4c/src/backend/riscv/codegen/variadic.cpp:32),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:9),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/variadic.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/variadic.rs:7)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `watch`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks the translated non-`F128` `float_ops.cpp::emit_float_binop_impl(...)`
  seam complete with the focused build/test proof lane above
- records that the remaining float-side work stays parked because
  `emit_f128_neg_impl(...)` still depends on wider F128 soft-float orchestration
- routes the next bounded RV64 packet to `comparison.cpp`:
  wire the translated non-`F128` `emit_float_cmp_impl(...)` owner entry plus
  the minimal shared `IrCmpOp` declaration surface it needs, while keeping
  integer compare, fused compare/branch, select, F128 compare, variadic, and
  broader floating-point expansion parked

Keep these families parked:

- `float_ops.cpp::emit_f128_neg_impl(...)`
- `comparison.cpp::emit_int_cmp_impl(...)`
- `comparison.cpp::emit_fused_cmp_branch_impl(...)`
- `comparison.cpp::emit_select_impl(...)`
- `comparison.cpp::emit_f128_cmp_impl(...)`
- `variadic.cpp`
- remaining broader `alu.cpp`, `memory.cpp`, globals GOT/TLS, and call-result
  work

## Rationale

The branch is still aligned with the plan: the single commit after the latest
lifecycle checkpoint lands exactly the float-binop packet that the runbook
named, and the focused proof lane passes. The route is stale only because
canonical lifecycle state still points at a slice that is already in `HEAD`.

At this checkpoint the bounded scalar float-binop seam is exhausted. The next
float-side body immediately widens into parked F128 soft-float work, and the
variadic lane still requires state that the shared RV64 C++ surface does not
expose. In contrast, `comparison.cpp` already contains the non-`F128` float
predicate tables and operand-order logic, so the next bounded owner entry can
stay on the same helper-sized integration standard as the landed float-binop
slice.
