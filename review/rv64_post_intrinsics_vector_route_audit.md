# RV64 Post-Intrinsics-Vector Route Audit

Date: 2026-04-13
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `99e47173`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 intrinsics lane to vector helper packet`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit naming the active packet, and the
  only execution diff on top of it is the just-landed bounded vector-helper
  intrinsics slice at `334a1fb0`
- commits since base: `1`

## Validation Observed

- reviewed the implementation delta for
  `src/backend/riscv/codegen/intrinsics.cpp` and
  `tests/backend/backend_shared_util_tests.cpp`
- reviewed proof evidence from `rv64_intrinsics_vector_build.log` and
  `rv64_intrinsics_vector_test.log`
- observed that the build log rebuilt the owned intrinsics/test objects and
  relinked `backend_shared_util_tests`, while the filtered test log is empty
  because the targeted intrinsics run passed without emitting failure text

## Findings

- Medium: canonical lifecycle state now points at a packet that `HEAD` already
  landed. `plan.md` and `todo.md` still name the vector-helper-backed
  intrinsics packet as active even though commit `334a1fb0` already wired
  `Pcmpeqb128`, `Pcmpeqd128`, `Psubusb128`, `Psubsb128`, `Por128`, `Pand128`,
  `Pxor128`, and `Pmovmskb128` through the shared RV64 dispatcher surface with
  focused intrinsics proof: [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:355),
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:3264).
- Medium: the current intrinsics route is still not exhausted. The RV64
  dispatcher now reaches the vector-helper-backed branches through
  `Pmovmskb128`, but it still falls straight to `default` after that point, so
  the adjacent translated `SetEpi8` and `SetEpi32` splat cases remain the next
  unconsumed owner work in the same software-SIMD family:
  [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:390),
  [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:120),
  [src/backend/riscv/codegen/README.md](/workspaces/c4c/src/backend/riscv/codegen/README.md:639).
- Medium: a blocked broader-route checkpoint would be premature. The translated
  Rust owner still places `SetEpi8` and `SetEpi32` immediately before the
  later CRC32 and scalar frame/return/thread-pointer and sqrt/fabs families,
  and both splat cases stay inside the already-landed `dest_ptr` /
  `operand_to_t0(...)` / storeback surface without needing new shared
  declarations. Skipping directly to CRC/scalar work, or blocking for a fresh
  broader-route decision now, would jump over a narrower same-file packet that
  is still plainly visible: [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:136),
  [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:151),
  [src/backend/riscv/codegen/README.md](/workspaces/c4c/src/backend/riscv/codegen/README.md:640).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Do not issue another execution packet from the stale lifecycle state.

Rewrite `plan.md` / `todo.md` first so canonical state:

- marks the bounded vector-helper-backed intrinsics packet as complete and
  records commit `334a1fb0` plus the focused `rv64_intrinsics_vector_build.log`
  / `rv64_intrinsics_vector_test.log` proof lane
- routes the next execution packet to the bounded translated
  `src/backend/riscv/codegen/intrinsics.cpp` splat seam only:
  `SetEpi8` plus `SetEpi32`

Keep these families parked:

- `Crc32_8`, `Crc32_16`, `Crc32_32`, and `Crc32_64`
- `FrameAddress`, `ReturnAddress`, and `ThreadPointer`
- `SqrtF64`, `SqrtF32`, `FabsF64`, and `FabsF32`
- x86-only fallback branches after the scalar builtins
- `peephole.cpp`

## Rationale

The branch has not drifted. The landed vector-helper packet did exactly what
the latest lifecycle checkpoint asked for, and the focused proof evidence
matches that bounded slice. Unlike the earlier blocked post-dispatch state,
the remaining live translated intrinsics inventory still exposes one adjacent
same-family packet before the lane broadens: byte and dword splat emission for
`SetEpi8` and `SetEpi32`. Those cases stay inside the existing owner file and
the already-landed operand/destination pointer surface, so canonical lifecycle
should advance to that narrow splat packet rather than block for a broader
route decision or jump ahead to CRC/scalar builtin work.
