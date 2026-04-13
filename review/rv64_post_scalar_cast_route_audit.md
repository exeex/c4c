# RV64 Post-Scalar-Cast Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `0bc59bfc`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to scalar cast owner seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan, and the
  only execution commit since then is the routed scalar cast owner packet
- commits since base: `1`
  (`3cadaafb rv64: wire scalar cast owner seam`)

## Scope Reviewed

- `src/backend/riscv/codegen/cast_ops.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/cast_ops.rs`

## Validation Checked

- `cmake --build build --target backend_shared_util_tests`
- `cmake --build build --target c4cll`
- `./build/backend_shared_util_tests riscv_codegen_header_exports_translated_cast_surface`
- `./build/backend_shared_util_tests riscv_translated_cast_emits_bounded_f128_text`
- `./build/backend_shared_util_tests riscv_translated_cast_emits_bounded_scalar_text`

Current log paths:

- `rv64_scalar_cast_build.log`
- `rv64_scalar_cast_c4cll_build.log`
- `rv64_scalar_cast_header_test.log`
- `rv64_scalar_cast_f128_test.log`
- `rv64_scalar_cast_test.log`

## Findings

- Medium: the landed packet cleanly completes the routed scalar cast owner
  seam. `cast_ops.cpp` now translates the non-`F128`/non-`I128` scalar cast
  instruction-selection body and replaces the prior non-F128 throw with the
  bounded operand-load / cast / store fallback that the Rust owner uses for
  scalar casts
  (`src/backend/riscv/codegen/cast_ops.cpp`,
  `src/backend/riscv/codegen/riscv_codegen.hpp`).
- Medium: proof for the routed seam stays appropriately focused. The new
  shared-util coverage checks signed-int to float, float to unsigned-int,
  float widen, and the RV64-specific `U32 -> I32` ABI sign-extension path,
  while the focused `c4cll` rebuild confirms the owner file still compiles on
  the main surface
  (`tests/backend/backend_shared_util_tests.cpp`).
- Medium: canonical lifecycle state is stale again. `plan.md` and `todo.md`
  still present the scalar cast owner packet as the next execution step even
  though commit `3cadaafb` already landed it, so more execution would stack on
  top of an out-of-date checkpoint.
- Medium: there is no equally narrow follow-on left in the current cast-owner
  lane. After the landed scalar packet, the remaining visible cast follow-on
  widens into shared default-cast / i128-cast work rather than another
  helper-sized or similarly bounded `cast_ops.cpp` packet, while the other
  parked families remain separate broader-route options
  (`ref/claudes-c-compiler/src/backend/riscv/codegen/cast_ops.rs`,
  `src/backend/riscv/codegen/cast_ops.cpp`).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks commit `3cadaafb` as the landed translated scalar cast owner seam with
  the focused proof lane above
- records that translated non-`F128`/non-`I128` scalar casts are now truthful
  through `src/backend/riscv/codegen/cast_ops.cpp`
- blocks the lane pending a fresh broader-route decision instead of issuing
  another execution packet immediately, because the remaining visible cast
  follow-on now widens into shared default-cast / i128-cast work or into
  other already-parked broader families

Keep these families parked:

- shared default-cast / i128-cast follow-on work
- broader `memory.cpp` typed-indirect and broader GEP owner work
- `globals.cpp::emit_tls_global_addr_impl(...)`
- broader `calls.cpp` outgoing-call lowering and F128/I128 result-store work
- integer-owner, fused-branch, select, and broader comparison follow-on work

## Rationale

The only execution commit since `0bc59bfc` does exactly what that lifecycle
checkpoint asked for: it lands the translated scalar cast instruction body and
the bounded non-F128 fallback path, then proves the new owner surface with
focused shared-util coverage plus a `c4cll` rebuild. This is a coherent
acceptance boundary, not drift.

The next honest action is not to guess another narrow executor packet in the
same file. The remaining visible cast work is broader shared default-cast /
i128 territory, and the other parked families are still broader route choices.
That makes this another lifecycle rewrite point, not another direct execution
step.
