# RV64 Post-Intrinsics-Dispatch Broader Route Audit

- Review scope: `plan.md`, `todo.md`, `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`, `src/backend/riscv/codegen/intrinsics.cpp`, `src/backend/riscv/codegen/peephole.cpp`, `src/backend/riscv/codegen/riscv_codegen.hpp`, `tests/backend/backend_shared_util_tests.cpp`, `ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs`, and git history for the current blocked checkpoint
- Chosen review base: `8fce9ffa` (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 intrinsics lane after first direct dispatcher packet`)
- Why this base is correct: `scripts/plan_change_gap.sh` reports `8fce9ffa` as the latest canonical lifecycle checkpoint, and that commit is the plan/todo rewrite that accepted commits `2f67b840` and `13bb33cc` as the completed first direct intrinsics dispatcher packet while explicitly blocking the lane for a fresh broader-route decision. This audit is about choosing that next route from the blocked checkpoint itself, so the blocked lifecycle commit is the right base.
- Commits reviewed since base: `0`

## Findings

### 1. The branch is still truthfully parked at the blocked post-dispatch checkpoint, so another lifecycle rewrite is required before more execution.

- `git diff 8fce9ffa..HEAD` is empty and `git rev-list --count 8fce9ffa..HEAD` is `0`, so there is no implementation drift to absorb before choosing the next route.
- Canonical state already records that the first direct intrinsics dispatcher packet is complete and that the lane is blocked pending a broader-route decision across the remaining intrinsics families: [plan.md](/workspaces/c4c/plan.md:86), [plan.md](/workspaces/c4c/plan.md:90), [todo.md](/workspaces/c4c/todo.md:26).
- Issuing another executor packet directly from the current lifecycle state would therefore be route drift rather than execution of an already-selected packet.

### 2. The nearest truthful continuation is the vector-helper-backed intrinsics family, because that helper surface is already landed, declared, and proofed but not yet consumed by `emit_intrinsic_rv(...)`.

- `intrinsics.cpp` already contains the parked helper bodies for the vector-shaped translated operations the activation packet was meant to expose: `emit_rv_binary_128(...)`, `emit_rv_cmpeq_bytes()`, `emit_rv_cmpeq_dwords()`, `emit_rv_psubusb_8bytes(...)`, `emit_rv_psubsb_128()`, and `emit_rv_pmovmskb()` all exist in-tree today, while the dispatcher still stops after the fence/non-temporal/load-store family and falls through to `default`: [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:39), [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:66), [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:81), [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:139), [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:202), [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:215), [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:296), [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:348).
- The current shared-util coverage already proves that exact helper surface is alive and reachable through the landed activation seam, while the direct dispatcher coverage currently stops at the first fence/non-temporal/load-store packet. The next honest continuation is therefore to consume the helper family that activation deliberately exposed, not to skip over it: [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:3190), [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:3229).
- The translated Rust source places that same vector family immediately after the already-landed fence/non-temporal/load-store packet and before the later CRC, frame/return/thread-pointer, sqrt/fabs, and x86-only fallback branches. Reusing that family boundary in lifecycle keeps the C++ route aligned with the translated owner structure rather than inventing a new split: [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:77), [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:116), [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:151), [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:197), [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:219), [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:255).
- The repo documentation also describes the RV64 intrinsics surface as one family containing fence/sqrt/fabs/CRC/frame-pointer logic alongside software SSE-equivalent 128-bit SIMD helpers. The helper-backed vector subfamily is the part most directly staged by the activation packet already on disk: [src/backend/riscv/codegen/README.md](/workspaces/c4c/src/backend/riscv/codegen/README.md:67).

### 3. The other visible candidates should stay parked for now because they either skip the staged helper seam or are not truthful runtime work on the active backend path.

- CRC32 and the scalar frame/return/thread-pointer and sqrt/fabs branches are real translated follow-on work, but they do not consume the already-landed vector helper surface that the activation packet and helper tests were explicitly preparing. Choosing them next would leave the staged helper family parked again even though it is the nearest unconsumed continuation in the same owner file: [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:151), [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:197), [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:219).
- The x86-only fallback branches remain poor route candidates. The translated source itself treats them as should-not-happen compatibility branches that merely zero a destination if they ever surface on RV64, so they are not the next truthful family to activate while real helper-backed work is still waiting: [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:255).
- `peephole.cpp` should remain parked. Canonical state already says it is a later native-emitter/finalization route, not the next intrinsics continuation, and nothing in the current blocked checkpoint changes that judgment: [plan.md](/workspaces/c4c/plan.md:105), [todo.md](/workspaces/c4c/todo.md:31).

## Judgments

- Plan-alignment judgment: `on track`
- Technical-debt judgment: `action needed`
- Route judgment: the blocked post-dispatch checkpoint is still correct, and the next truthful broader lifecycle route is the vector-helper-backed translated RV64 intrinsics family rather than CRC, scalar builtin branches, x86-only fallbacks, or `peephole.cpp`.

## Route Decision

- Selected next broader lifecycle route: the vector-helper-backed translated `src/backend/riscv/codegen/intrinsics.cpp` family centered on making `emit_intrinsic_rv(...)` truthful for `Pcmpeqb128`, `Pcmpeqd128`, `Psubusb128`, `Psubsb128`, `Por128`, `Pand128`, `Pxor128`, and `Pmovmskb128` by consuming the already-landed helper surface.
- Narrowest honest continuation from the blocked checkpoint: one lifecycle rewrite that keeps work inside the active intrinsics owner file and narrows the next executor packet to those helper-backed branches plus the focused `backend_shared_util_tests intrinsics` proof lane, without widening into later CRC, scalar builtin, or x86-only fallback work.
- Parked families:
  - keep CRC32 and scalar `FrameAddress` / `ReturnAddress` / `ThreadPointer` branches parked until the helper-backed vector packet lands
  - keep scalar `SqrtF64` / `SqrtF32` / `FabsF64` / `FabsF32` parked until the vector-helper packet is complete
  - keep x86-only fallback branches parked because they are compatibility cleanup, not the nearest truthful RV64 continuation
  - keep `peephole.cpp` parked until a later native-emitter / backend-finalization route makes it the nearest truthful continuation

## Recommendation

- Reviewer recommendation: `rewrite plan/todo before more execution`
- Rationale: the branch is aligned with the blocked post-dispatch checkpoint, but the current route is consumed. Canonical lifecycle should advance to the vector-helper-backed intrinsics family before any executor packet is issued.
