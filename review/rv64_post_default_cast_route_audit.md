# RV64 Post-Default-Cast Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `f46e9293`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to shared default-cast seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan, and the
  only execution commit since then is the routed shared default-cast /
  i128-cast packet
- commits since base: `1`
  (`31540e07 rv64: wire shared default-cast seam`)

## Scope Reviewed

- `src/backend/traits.cpp`
- `src/backend/riscv/codegen/cast_ops.cpp`
- `src/backend/riscv/codegen/i128_ops.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `CMakeLists.txt`
- `ref/claudes-c-compiler/src/backend/traits.rs`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/cast_ops.rs`

## Validation Checked

- `cmake --build build --target backend_shared_util_tests c4cll`
- `./build/backend_shared_util_tests riscv_codegen_header_exports_translated_cast_surface`
- `./build/backend_shared_util_tests riscv_translated_cast_emits_bounded_f128_text`
- `./build/backend_shared_util_tests riscv_translated_cast_emits_bounded_scalar_text`

Current log paths:

- `rv64_default_cast_build.log`
- `rv64_default_cast_header_test.log`
- `rv64_default_cast_f128_test.log`
- `rv64_default_cast_scalar_test.log`

## Findings

- Medium: the landed packet cleanly completes the routed shared default-cast /
  i128-cast seam. `cast_ops.cpp::emit_cast_impl(...)` now delegates all
  non-F128 cast follow-on through the shared `emit_cast_default(...)` helper,
  while `traits.cpp` owns the bounded default path and the minimum RV64
  accumulator-pair, sign/zero-extension, and compiler-rt cast helpers are now
  reachable through `riscv_codegen.hpp` and `i128_ops.cpp`
  (`src/backend/traits.cpp`,
  `src/backend/riscv/codegen/cast_ops.cpp`,
  `src/backend/riscv/codegen/i128_ops.cpp`,
  `src/backend/riscv/codegen/riscv_codegen.hpp`,
  `CMakeLists.txt`).
- Medium: proof for the routed seam stays appropriately focused. The shared
  util coverage checks declaration reachability plus bounded emitted text for
  the already-landed F128 families, narrow-to-i128 widening, float-to-i128,
  i128-to-float, and scalar fallback paths without claiming broader i128 owner
  completion
  (`tests/backend/backend_shared_util_tests.cpp`).
- Medium: canonical lifecycle state is stale again. `plan.md` and `todo.md`
  still present the shared default-cast seam as the next execution packet even
  though commit `31540e07` already landed it, so more execution would stack on
  top of an out-of-date checkpoint.
- Medium: there is no equally narrow cast follow-on left after this seam.
  The remaining visible work widens into broader i128 owner bodies beyond the
  helper surface just landed, or into other already-parked broader families:
  typed-indirect / broader GEP memory work, TLS globals, broader outgoing-call
  lowering, and integer-owner / fused-branch / select comparison work
  (`src/backend/riscv/codegen/i128_ops.cpp`,
  `src/backend/riscv/codegen/globals.cpp`,
  `src/backend/riscv/codegen/calls.cpp`,
  `src/backend/riscv/codegen/comparison.cpp`).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks commit `31540e07` as the landed shared `src/backend/traits.cpp`
  default-cast seam plus the minimum RV64 `cast_ops.cpp` /
  `riscv_codegen.hpp` / `i128_ops.cpp` helper surface with the focused proof
  lane above
- records that translated RV64 shared default-cast / i128 helper families are
  now truthful through the shared seam and the bounded helper exports
- blocks the lane pending a fresh broader-route decision instead of issuing
  another execution packet immediately, because the remaining visible follow-on
  widens into broader i128 owner work or other already-parked broader families

Keep these families parked:

- broader i128 owner work beyond the landed helper surface
- broader `memory.cpp` typed-indirect and broader GEP owner work
- `globals.cpp::emit_tls_global_addr_impl(...)`
- broader `calls.cpp` outgoing-call lowering and I128/F128 result-store work
- integer-owner, fused-branch, select, and broader comparison follow-on work

## Rationale

The only execution commit since `f46e9293` does exactly what that lifecycle
checkpoint asked for: it lands the shared default-cast seam in `traits.cpp`,
wires the minimum RV64 helper surface needed to use it, and proves the seam
with focused shared-util coverage plus a `c4cll` rebuild. This is a coherent
acceptance boundary, not drift.

The next honest action is not to guess another executor packet in the same
lane. The cast route that was visible from the Rust reference is now consumed,
and what remains is broader i128 owner work or a pivot to other broader parked
families. That makes this another lifecycle rewrite point, not another direct
execution step.
