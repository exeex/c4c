# RV64 Post-F128-Cast Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `4e4117b7`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to shared f128 cast seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan, and the
  only execution commit since then is the routed shared F128 cast packet
- commits since base: `1`
  (`4c8e4a93 rv64: wire shared f128 cast seam`)

## Scope Reviewed

- `src/backend/f128_softfloat.cpp`
- `src/backend/f128_softfloat.hpp`
- `src/backend/riscv/codegen/cast_ops.cpp`
- `src/backend/riscv/codegen/f128.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `ref/claudes-c-compiler/src/backend/f128_softfloat.rs`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/cast_ops.rs`

## Validation Checked

- `cmake --build build --target backend_shared_util_tests`
- `./build/backend_shared_util_tests riscv_codegen_header_exports_translated_cast_surface`
- `./build/backend_shared_util_tests riscv_translated_cast_emits_bounded_f128_text`

Current log paths:

- `rv64_f128_cast_build.log`
- `rv64_f128_cast_header_test.log`
- `rv64_f128_cast_test.log`

## Findings

- Medium: the landed packet cleanly completes the routed shared F128 cast
  seam. The shared soft-float layer now owns the translated RV64 integer/F32/F64
  to-from `IrType::F128` cast orchestration, and `cast_ops.cpp::emit_cast_impl(...)`
  stops throwing on those F128 families by routing them through
  `f128_emit_cast(...)`
  (`src/backend/f128_softfloat.cpp`,
  `src/backend/f128_softfloat.hpp`,
  `src/backend/riscv/codegen/cast_ops.cpp`,
  `src/backend/riscv/codegen/f128.cpp`,
  `src/backend/riscv/codegen/riscv_codegen.hpp`).
- Medium: proof for the routed seam stays appropriately focused. The new
  shared-util coverage checks both declaration reachability and bounded emitted
  text for unsigned-int to F128, F128 to F64, and F128 to signed-int paths
  without claiming broader scalar-cast or i128-cast completion
  (`tests/backend/backend_shared_util_tests.cpp`).
- Medium: canonical lifecycle state is stale again. `plan.md` and `todo.md`
  still present the shared F128 cast seam as the next execution packet even
  though commit `4c8e4a93` already landed it, so more execution would stack on
  top of an out-of-date checkpoint.
- Medium: the shared F128 seam is exhausted again; the smallest truthful
  follow-on is the translated scalar non-`F128`/non-`I128` cast owner packet,
  not another shared soft-float step. In the Rust reference, the remaining
  visible work in `cast_ops.rs` after the F128 override is
  `emit_cast_instrs_impl(...)` plus the scalar `emit_cast_impl(...)` fallback.
  The current C++ owner still throws for every non-F128 cast, but it already
  has the bounded `operand_to_t0(...)` and `store_t0_to(...)` surface needed
  to make the non-`I128` scalar path real without widening into the broader
  shared default-cast / i128 lane
  (`ref/claudes-c-compiler/src/backend/riscv/codegen/cast_ops.rs`,
  `src/backend/riscv/codegen/cast_ops.cpp`,
  `src/backend/riscv/codegen/emit.cpp`).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks commit `4c8e4a93` as the landed shared `backend/f128_softfloat.cpp`
  F128 cast seam with the focused proof lane above
- records that the translated RV64 F128 cast families are now truthful through
  the shared soft-float cast dispatch
- routes the next lifecycle stage to the broader translated scalar cast owner
  packet in `src/backend/riscv/codegen/cast_ops.cpp`, centered on
  `emit_cast_instrs_impl(...)` plus only the minimum `emit_cast_impl(...)`
  fallback needed to make non-`F128`/non-`I128` scalar casts real

Keep these families parked:

- shared default-cast / i128-cast follow-on work
- broader `memory.cpp` typed-indirect and broader GEP owner work
- `globals.cpp::emit_tls_global_addr_impl(...)`
- broader `calls.cpp` outgoing-call lowering and F128/I128 result-store work
- integer-owner, fused-branch, select, and broader comparison follow-on work

## Rationale

The only execution commit since `4e4117b7` does exactly what that lifecycle
checkpoint asked for: it lands shared F128 cast orchestration and proves the
new declaration and emitted-text surface with focused shared-util coverage.
This is a coherent acceptance boundary, not drift.

The next honest action is not to keep pointing at the already-landed shared
soft-float cast packet. The remaining visible follow-on in the translated RV64
cast owner is now scalar cast instruction selection and the bounded non-F128
fallback path. That is still broader than the shared F128 seam that just
landed, but it is materially narrower than widening straight into shared
default-cast / i128 work or pivoting to unrelated parked families.
