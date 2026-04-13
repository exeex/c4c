# RV64 Post-Intrinsics-Splat Route Audit

Date: 2026-04-13
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `499bdcb4`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: advance rv64 intrinsics route to splat packet`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit naming the current active packet,
  and the only execution diff on top of it is the just-landed bounded splat
  slice at `451035db`
- commits since base: `1`

## Validation Observed

- reviewed the implementation delta for
  `src/backend/riscv/codegen/intrinsics.cpp` and
  `tests/backend/backend_shared_util_tests.cpp`
- reviewed proof evidence from `rv64_intrinsics_splat_build.log` and
  `rv64_intrinsics_splat_test.log`
- observed that the build log rebuilt the owned intrinsics/test objects and
  relinked `backend_shared_util_tests`, while the filtered test log is empty
  because the targeted intrinsics run passed without emitting failure text

## Findings

- Medium: canonical lifecycle state is now stale again. `plan.md` and
  `todo.md` still name the `SetEpi8` / `SetEpi32` splat packet as active even
  though commit `451035db` already wired both cases through the existing
  destination-pointer / `operand_to_t0(...)` surface and the dispatcher test
  now exercises those paths:
  [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:397),
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:3280).
- Medium: the current intrinsics lane is still not exhausted. Immediately
  after the landed splat cases, the translated owner continues with the
  software `Crc32_8` / `Crc32_16` / `Crc32_32` / `Crc32_64` family, which
  stays inside the same `emit_intrinsic_rv(...)` owner surface and uses the
  already-landed scalar operand/result path rather than requiring a broader
  shared-surface expansion:
  [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:151).
- Medium: a blocked checkpoint or lane switch would be premature. The next
  visible translated inventory after CRC widens into frame/return/thread-
  pointer builtins and scalar sqrt/fabs work, but the adjacent CRC family is a
  narrower same-file continuation than jumping to another owner file or
  parking the lane now:
  [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:197),
  [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:219).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Do not issue another execution packet from the stale lifecycle state.

Rewrite `plan.md` / `todo.md` first so canonical state:

- marks the bounded `SetEpi8` / `SetEpi32` splat packet as complete and
  records commit `451035db` plus the focused
  `rv64_intrinsics_splat_build.log` / `rv64_intrinsics_splat_test.log` proof
  lane
- routes the next execution packet to the bounded translated
  `src/backend/riscv/codegen/intrinsics.cpp` CRC family only:
  `Crc32_8`, `Crc32_16`, `Crc32_32`, and `Crc32_64`

Keep these families parked:

- `FrameAddress`, `ReturnAddress`, and `ThreadPointer`
- `SqrtF64`, `SqrtF32`, `FabsF64`, and `FabsF32`
- x86-only fallback branches after the scalar builtins
- `peephole.cpp`

## Rationale

The branch has not drifted. Commit `451035db` does exactly what the latest
lifecycle checkpoint asked for by wiring the translated byte and dword splat
branches and extending focused dispatcher coverage on the same owner seam.
Unlike the earlier post-vector state, the remaining live translated intrinsics
inventory still exposes one adjacent same-file continuation before the route
broadens: the software CRC32 family. Those cases stay inside the existing
dispatcher, consume the already-landed scalar operand/result surface, and are
more truthful as the next packet than blocking now or switching to another
owner family.
