# RV64 Post-Float-Compare Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `389aac97`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to float compare seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan, and the
  only execution diff since then is the landed implementation slice for that
  exact packet
- commits since base: `1`
  (`9b0aefb4 riscv: wire translated float compare seam`)

## Validation Observed

- reviewed the implementation delta for `src/backend/riscv/codegen/comparison.cpp`,
  `src/backend/riscv/codegen/riscv_codegen.hpp`,
  `tests/backend/backend_shared_util_tests.cpp`, and `CMakeLists.txt`
- reviewed proof evidence from `rv64_float_cmp_build.log`,
  `rv64_float_cmp_header_shared_util.log`, and `rv64_float_cmp_shared_util.log`
- observed that the build log rebuilt `backend_shared_util_tests` and `c4cll`
  with the translated `comparison.cpp` owner in-tree, and both filtered
  shared-util logs are empty because the targeted tests passed without
  emitting failure text

## Findings

- Medium: canonical lifecycle state is stale, but the landed float-compare
  slice is aligned with the route and stays bounded. The shared RV64 surface
  gained only `IrCmpOp` plus the translated non-`F128`
  `emit_float_cmp_impl(...)` owner entry, `comparison.cpp` entered the active
  build lists, and the focused shared-util coverage proves the bounded scalar
  F32/F64 compare text path
  ([`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:326),
  [`src/backend/riscv/codegen/comparison.cpp`](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:56),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:391),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:926)).
- Medium: the remaining comparison inventory is not the next narrow packet.
  The untranslated follow-ons still imply wider shared state than the current
  RV64 header exposes: integer compare depends on compare-operand loading,
  fused compare/branch and select require fresh labels plus branch/cache
  bookkeeping, and the F128 compare path still delegates into soft-float
  orchestration
  ([`src/backend/riscv/codegen/comparison.cpp`](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:3),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:36),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:75),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:111),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:140)).
- Medium: globals, float-side follow-on work, and variadic work all remain
  wider than the next packet. `globals.cpp` still needs missing GOT/TLS
  state, `emit_f128_neg_impl(...)` still widens into the parked F128
  soft-float owner path, and `variadic.cpp` still depends on unshared
  va-list-state and pointer-location hooks
  ([`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:14),
  [`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:34),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/float_ops.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/float_ops.rs:49),
  [`src/backend/riscv/codegen/variadic.cpp`](/workspaces/c4c/src/backend/riscv/codegen/variadic.cpp:30)).
- Medium: the narrowest practical follow-on is back in `calls.cpp`, but only
  for the bounded non-`F128`/non-`I128` result-store path in
  `emit_call_store_result_impl(...)`. The translated Rust body already uses
  shared `IrType`, slot lookup, callee-saved naming, and S0-relative result
  stores for the scalar integer/F32/F64 cases; only the I128 and F128 subpaths
  still pull in pair-store or F128-tracking state that the current C++ header
  does not represent
  ([`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:90),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:457),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:476),
  [`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:310),
  [`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:338)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks the translated non-`F128` `comparison.cpp::emit_float_cmp_impl(...)`
  seam complete with the focused build and shared-util proof lane above
- records that the remaining comparison bodies stay parked because they still
  require wider compare-loader, label, branch-cache, or F128 soft-float state
- routes the next bounded RV64 packet to `calls.cpp`:
  wire only the translated non-`F128`/non-`I128`
  `emit_call_store_result_impl(...)` owner entry plus the minimal shared
  declaration surface it needs

Keep these families parked:

- `comparison.cpp::emit_int_cmp_impl(...)`
- `comparison.cpp::emit_fused_cmp_branch_impl(...)`
- `comparison.cpp::emit_select_impl(...)`
- `comparison.cpp::emit_f128_cmp_impl(...)`
- `float_ops.cpp::emit_f128_neg_impl(...)`
- `globals.cpp::emit_global_addr_impl(...)`
- `globals.cpp::emit_tls_global_addr_impl(...)`
- `variadic.cpp`
- `emit_call_f128_pre_convert_impl(...)`
- `emit_call_stack_args_impl(...)`
- `emit_call_reg_args_impl(...)`
- `emit_call_store_result_impl(...)` F128 and I128 paths
- remaining broader `alu.cpp` and `memory.cpp` owner work

## Rationale

The branch is still aligned with the plan: the single commit after the latest
lifecycle checkpoint lands exactly the float-compare packet that the runbook
named, and the focused proof evidence matches that bounded slice. The route is
stale only because canonical lifecycle state still points at a packet that is
already in `HEAD`.

At this checkpoint the bounded scalar comparison lane is exhausted. The
remaining comparison bodies all need shared state that is materially wider
than the current RV64 seam, and the previously parked globals, variadic, and
F128 float follow-ons remain blocked for the same reason. In contrast, the
scalar portion of `emit_call_store_result_impl(...)` can reuse the already
landed calls surface plus existing slot/reg helpers without reopening stack
argument materialization, register staging, GOT/TLS state, or F128 tracking.
