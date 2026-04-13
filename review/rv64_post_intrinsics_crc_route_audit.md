# RV64 Post-Intrinsics-CRC Route Audit

Date: 2026-04-13
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `fc741f14`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: advance rv64 intrinsics route to crc packet`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit naming the current active packet,
  and the only execution diff on top of it is the just-landed bounded CRC
  slice at `26a02a2d`
- commits since base: `1`

## Validation Observed

- reviewed the implementation delta for
  `src/backend/riscv/codegen/intrinsics.cpp` and
  `tests/backend/backend_shared_util_tests.cpp`
- reviewed proof evidence from `rv64_intrinsics_crc_build.log` and
  `rv64_intrinsics_crc_test.log`
- observed that the build log rebuilt the owned intrinsics/test objects and
  relinked `backend_shared_util_tests`, while the filtered test log is empty
  because the targeted intrinsics run passed without emitting failure text

## Findings

- Medium: canonical lifecycle state is stale again. `plan.md` and `todo.md`
  still name the `Crc32_8` / `Crc32_16` / `Crc32_32` / `Crc32_64` packet as
  active even though commit `26a02a2d` already wired those cases through the
  existing scalar operand/result path and the dispatcher test now exercises the
  translated CRC loop:
  [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:425),
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:3284).
- Medium: the current intrinsics lane is still not exhausted. Immediately
  after the landed CRC family, the translated owner continues with the scalar
  builtin trio `FrameAddress`, `ReturnAddress`, and `ThreadPointer`, which
  stays inside the same `emit_intrinsic_rv(...)` owner surface and consumes
  the already-landed scalar result path rather than requiring a broader shared
  expansion:
  [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:196).
- Medium: blocking now or switching lanes would be premature. The route only
  broadens after the scalar builtin trio into `SqrtF64`, `SqrtF32`,
  `FabsF64`, and `FabsF32`, followed by x86-only fallback branches, so the
  scalar builtin packet is the narrower same-file continuation:
  [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:216).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Do not issue another execution packet from the stale lifecycle state.

Rewrite `plan.md` / `todo.md` first so canonical state:

- marks the bounded CRC packet as complete and records commit `26a02a2d` plus
  the focused `rv64_intrinsics_crc_build.log` /
  `rv64_intrinsics_crc_test.log` proof lane
- routes the next execution packet to the bounded translated
  `src/backend/riscv/codegen/intrinsics.cpp` scalar builtin trio only:
  `FrameAddress`, `ReturnAddress`, and `ThreadPointer`

Keep these families parked:

- `SqrtF64`, `SqrtF32`, `FabsF64`, and `FabsF32`
- x86-only fallback branches after the scalar builtins
- `peephole.cpp`

## Rationale

The branch has not drifted. Commit `26a02a2d` does exactly what the latest
lifecycle checkpoint asked for by wiring the translated software CRC32 family
and extending focused dispatcher coverage on the same owner seam. The
remaining live translated intrinsics inventory still exposes one adjacent
same-file continuation before the route broadens: the scalar builtin trio for
frame address, return address, and thread pointer. Those cases stay inside the
existing dispatcher, consume the already-landed scalar result path, and are
more truthful as the next packet than blocking now or switching to another
owner family.
