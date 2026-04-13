# RV64 Post-F128-Memory Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `23a9f838`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to shared f128 memory seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan, and the
  only execution commit since then is the routed shared F128 memory packet
- commits since base: `1`
  (`d8a09656 rv64: wire shared f128 memory seam`)

## Scope Reviewed

- `src/backend/f128_softfloat.cpp`
- `src/backend/f128_softfloat.hpp`
- `src/backend/riscv/codegen/f128.cpp`
- `src/backend/riscv/codegen/memory.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `ref/claudes-c-compiler/src/backend/f128_softfloat.rs`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/memory.rs`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/cast_ops.rs`
- `src/backend/riscv/codegen/cast_ops.cpp`
- `CMakeLists.txt`

## Validation Checked

- `cmake --build build --target backend_shared_util_tests`
- `./build/backend_shared_util_tests riscv_codegen_header_exports_translated_f128_memory`
- `./build/backend_shared_util_tests riscv_translated_f128_memory`

Current log paths:

- `rv64_f128_memory_build.log`
- `rv64_f128_memory_header_test.log`
- `rv64_f128_memory_test.log`

## Findings

- Medium: the landed packet cleanly completes the routed shared F128 memory
  seam. `memory.cpp` now routes exactly the four `IrType::F128`
  store/load entry points through new shared `backend/f128_softfloat.cpp`
  entry points, and the RV64 side adds only the narrow address-register
  helpers that shared dispatch needed
  (`src/backend/f128_softfloat.cpp`,
  `src/backend/f128_softfloat.hpp`,
  `src/backend/riscv/codegen/f128.cpp`,
  `src/backend/riscv/codegen/memory.cpp`,
  `src/backend/riscv/codegen/riscv_codegen.hpp`).
- Medium: proof for the routed seam stays appropriately focused. The new
  shared-util coverage checks both declaration reachability and bounded emitted
  text for direct, indirect, and over-aligned F128 load/store paths without
  claiming broader cast or owner-family completion
  (`tests/backend/backend_shared_util_tests.cpp`).
- Medium: canonical lifecycle state is stale again. `plan.md` and `todo.md`
  still present the shared F128 memory seam as the next execution packet even
  though commit `d8a09656` already landed it, so more execution would stack on
  top of an out-of-date checkpoint.
- Medium: the next truthful follow-on is the broader shared F128 cast seam,
  not another memory packet. In the shared reference, the next major F128
  surface after store/load is `f128_emit_cast(...)`, and the translated RV64
  cast owner already delegates all F128 cast families there in the Rust
  source. The current C++ `src/backend/riscv/codegen/cast_ops.cpp` is still
  parked commented inventory and is not on the active RV64 build surface, so
  this is another lifecycle rewrite point rather than an immediate same-seam
  executor continuation
  (`ref/claudes-c-compiler/src/backend/f128_softfloat.rs`,
  `ref/claudes-c-compiler/src/backend/riscv/codegen/cast_ops.rs`,
  `src/backend/riscv/codegen/cast_ops.cpp`,
  `CMakeLists.txt`).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks commit `d8a09656` as the landed shared `backend/f128_softfloat.cpp`
  F128 memory seam with the focused proof lane above
- records that the translated RV64 `memory.cpp` F128 store/load entry points
  are now truthful through the shared soft-float memory dispatch
- routes the next lifecycle stage to the broader shared F128 cast seam:
  `f128_emit_cast(...)` plus only the minimum RV64 `cast_ops.cpp` owner/build
  surface needed to make the translated F128 cast dispatch real

Keep these families parked:

- broader `memory.cpp` typed-indirect and broader GEP owner work
- integer-owner, fused-branch, and select comparison-owner follow-on work
- `globals.cpp::emit_tls_global_addr_impl(...)`
- broader outgoing-call lowering and F128/I128 result-store work
- any broader RV64 shared-state expansion beyond what the cast route proves
  necessary

## Rationale

The only execution commit since `23a9f838` does exactly what that lifecycle
checkpoint asked for: it lands the shared F128 memory dispatch and proves the
new declaration and emitted-text surface with focused shared-util coverage.
This is a coherent acceptance boundary, not drift.

The next honest action is not to keep issuing executor packets against stale
memory wording. The shared reference moves next into cast orchestration, and
the translated RV64 cast owner already names that shared seam as its F128
override. Because the current C++ cast owner is still parked inventory rather
than live build surface, choosing that follow-on now requires another
lifecycle rewrite first.
