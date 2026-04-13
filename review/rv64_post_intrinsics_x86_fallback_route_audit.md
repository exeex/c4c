# RV64 Post-Intrinsics-X86-Fallback Route Audit

Date: 2026-04-13
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `ac08e26c`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: narrow rv64 intrinsics x86 fallback route after AES-CLMUL slice`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit naming the current active packet,
  and the only execution diff on top of it is the just-landed bounded
  remainder slice at `7c9e0776`
- commits since base: `1`

## Scope Reviewed

- `plan.md`
- `todo.md`
- `src/backend/riscv/codegen/intrinsics.cpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs`

## Validation Checked

- `cmake --build build --target backend_shared_util_tests`
- `./build/backend_shared_util_tests intrinsics`

Current log paths:

- `rv64_intrinsics_x86_fallback_remainder_build.log`
- `rv64_intrinsics_x86_fallback_remainder_test.log`

## Findings

- Medium: the landed packet matches the active narrowed route. Commit
  `7c9e0776` wires the remaining translated x86-only zero-destination
  compatibility cases in `emit_intrinsic_rv(...)` through the already-landed
  destination-pointer surface without widening into `peephole.cpp` or another
  owner family
  ([`src/backend/riscv/codegen/intrinsics.cpp`](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:318),
  [`src/backend/riscv/codegen/intrinsics.cpp`](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:533)).
- Medium: focused proof remains aligned with the routed seam. The shared-util
  dispatcher test now exercises the full translated x86-only fallback block
  and checks that every routed compatibility case zeroes both 64-bit halves of
  the destination vector
  ([`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:3306),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:3473)).
- Medium: canonical lifecycle state is stale again. `plan.md` and `todo.md`
  still present the narrowed fallback remainder as an active execution packet
  even though commit `7c9e0776` already landed it
  ([`plan.md`](/workspaces/c4c/plan.md:122),
  [`todo.md`](/workspaces/c4c/todo.md:60)).
- Medium: there is no equally narrow direct follow-on left on the current
  RV64 intrinsics owner lane. In the translated Rust owner, the x86-only
  fallback block is the tail of `emit_intrinsic_rv(...)`, so the direct
  intrinsics route is consumed at this seam rather than exposing another
  adjacent same-file packet
  ([`ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:255),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:292)).
- Medium: the remaining visible follow-on work is still broader parked
  inventory rather than another immediate executor packet. Current lifecycle
  state already parks broader atomics follow-on, broader calls/comparison/
  memory work, and `peephole.cpp`, and the just-landed remainder does not
  narrow one of those families enough to choose it without a fresh
  broader-route decision
  ([`plan.md`](/workspaces/c4c/plan.md:131),
  [`todo.md`](/workspaces/c4c/todo.md:107)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks commit `7c9e0776` as the landed remainder of the translated x86-only
  fallback block with the focused proof lane above, on top of the earlier
  AES/CLMUL sub-slice at `813bb64f`
- records that the direct `emit_intrinsic_rv(...)` x86-only compatibility
  block is now complete through the shared zero-destination pointer surface
- blocks the lane pending a fresh broader-route decision instead of issuing
  another executor packet immediately, because the remaining visible follow-on
  work now widens into already-parked broader families rather than another
  equally narrow intrinsics-owner slice

Keep these families parked:

- `peephole.cpp`
- broader atomics follow-on
- broader `calls.cpp`, `comparison.cpp`, and `memory.cpp` follow-on work
- any non-intrinsics follow-on outside the now-complete x86-only fallback block

## Rationale

The only execution commit since `ac08e26c` does exactly what that lifecycle
checkpoint asked for: it lands the adjacent remainder of the translated
x86-only fallback block and proves the route with the named focused checks.
This is a coherent acceptance boundary, not drift.

The next honest action is not to guess another executor packet off stale
state. The translated RV64 intrinsics owner ends at this x86-only fallback
block, and the remaining families named by current lifecycle state are still
broader parked routes rather than another adjacent same-file continuation.
That makes this another blocked-checkpoint rewrite point, with a later
broader-route audit needed before more execution.
