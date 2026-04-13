# RV64 Post-Stale-Memory Broader Route Audit

- Review scope: `plan.md`, `todo.md`, `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`, `review/rv64_step27_memory_route_sanity_audit.md`, `src/backend/riscv/codegen/atomics.cpp`, `src/backend/riscv/codegen/inline_asm.cpp`, `src/backend/riscv/codegen/intrinsics.cpp`, `src/backend/riscv/codegen/peephole.cpp`, `src/backend/riscv/codegen/asm_emitter.cpp`, `src/backend/backend.cpp`, `CMakeLists.txt`, and git history for the current blocked checkpoint
- Chosen review base: `1686697a` (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane after stale memory route activation`)
- Why this base is correct: `scripts/plan_change_gap.sh` reports `1686697a` as the latest canonical lifecycle checkpoint, and that commit is the plan/todo rewrite that accepted `review/rv64_step27_memory_route_sanity_audit.md` and explicitly blocked the lane again after rejecting the stale Step 27 memory activation. This audit is about choosing the next truthful route from that blocked checkpoint itself, so the blocked lifecycle commit is the right base.
- Commits reviewed since base: `0`

## Findings

### 1. The branch is still truthfully parked at the blocked post-stale-memory checkpoint, so any further execution still requires another lifecycle rewrite first.

- `git diff 1686697a..HEAD` is empty and `git rev-list --count 1686697a..HEAD` is `0`, so there is no implementation drift to absorb before choosing the next route.
- Canonical state already records that the Step 27 memory activation was stale and that the lane is blocked again pending a fresh broader-route decision: [review/rv64_step27_memory_route_sanity_audit.md](/workspaces/c4c/review/rv64_step27_memory_route_sanity_audit.md:22), [review/rv64_step27_memory_route_sanity_audit.md](/workspaces/c4c/review/rv64_step27_memory_route_sanity_audit.md:31), [todo.md](/workspaces/c4c/todo.md:10), [ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md](/workspaces/c4c/ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md:491).
- Issuing an executor packet from the current lifecycle state would therefore be route drift rather than execution of an already-selected packet.

### 2. The nearby memory, calls, comparison, globals, variadic, and current inline-asm helper surfaces are not the narrowest honest continuation from this checkpoint.

- The stale-memory audit already shows that the previously selected `memory.cpp` typed-indirect / broader GEP family was activated onto materially landed code and tests, so reopening that route would repeat the same lifecycle mistake rather than advancing the branch: [review/rv64_step27_memory_route_sanity_audit.md](/workspaces/c4c/review/rv64_step27_memory_route_sanity_audit.md:10), [review/rv64_step27_memory_route_sanity_audit.md](/workspaces/c4c/review/rv64_step27_memory_route_sanity_audit.md:16).
- The earlier broader-route audit that chose memory had already treated the visible variadic, calls, comparison, globals, and current ALU lanes as exhausted at that route level; nothing new has landed since then that would make one of those families the next honest continuation now: [review/rv64_post_variadic_owner_broader_route_audit.md](/workspaces/c4c/review/rv64_post_variadic_owner_broader_route_audit.md:16), [review/rv64_post_variadic_owner_broader_route_audit.md](/workspaces/c4c/review/rv64_post_variadic_owner_broader_route_audit.md:34).
- `inline_asm.cpp` is not a clean next route either. Its visible owner helpers are the same constraint classification and operand-substitution surface that is already live in `asm_emitter.cpp` and already covered by focused shared-util tests, so promoting `inline_asm.cpp` next would risk another stale duplicate activation rather than a new truthful family: [src/backend/riscv/codegen/inline_asm.cpp](/workspaces/c4c/src/backend/riscv/codegen/inline_asm.cpp:64), [src/backend/riscv/codegen/inline_asm.cpp](/workspaces/c4c/src/backend/riscv/codegen/inline_asm.cpp:121), [src/backend/riscv/codegen/asm_emitter.cpp](/workspaces/c4c/src/backend/riscv/codegen/asm_emitter.cpp:98), [src/backend/riscv/codegen/asm_emitter.cpp](/workspaces/c4c/src/backend/riscv/codegen/asm_emitter.cpp:132), [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:4409).

### 3. The narrowest honest broader continuation now left on the board is the translated `atomics.cpp` activation family.

- `atomics.cpp` is still pure parked translated inventory: it remains only a commented method-shaped mirror of the ref Rust owner surface and has no live owner bodies yet, unlike the stale memory route or the already-exposed asm-emitter helper surface: [src/backend/riscv/codegen/atomics.cpp](/workspaces/c4c/src/backend/riscv/codegen/atomics.cpp:1).
- The active build lists still omit `atomics.cpp` entirely while compiling the already-landed RV64 owner cluster, which means atomics has not yet even crossed the first shared build boundary that memory/calls/comparison/variadic already crossed: [CMakeLists.txt](/workspaces/c4c/CMakeLists.txt:220), [CMakeLists.txt](/workspaces/c4c/CMakeLists.txt:459).
- `intrinsics.cpp` is less ready as the next route than atomics because its file itself says the real dispatch layer still depends on shared `RiscvCodegen`, `Operand`, `Value`, and `IntrinsicOp` surfaces that are not yet represented in a common C++ header, so choosing it now would imply a wider shared-surface expansion than the first atomics activation step: [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:220).
- `peephole.cpp` is also not the narrowest continuation. It does expose a real optimizer entry point, but the current RV64 backend still does not emit a native RV64 assembly path in `backend.cpp`; prepared LIR still falls back to LLVM text and BIR rendering, so landing RV64 peephole next would optimize a path the backend does not yet consume: [src/backend/riscv/codegen/peephole.cpp](/workspaces/c4c/src/backend/riscv/codegen/peephole.cpp:1191), [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:65), [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:89).
- By elimination, atomics is the remaining translated RV64 codegen family that is both materially unintegrated and not obviously superseded by an already-landed overlapping seam. The truthful next route is therefore to activate that family through the minimum shared header/build surface needed for its first bounded owner packet, rather than to guess another packet inside stale memory or duplicate helper territory.

## Judgments

- Plan-alignment judgment: `on track`
- Technical-debt judgment: `action needed`
- Route judgment: the current blocked checkpoint is still correct, and the next truthful broader lifecycle route is no longer inside `memory.cpp` or another already-landed helper family; it is the broader translated `src/backend/riscv/codegen/atomics.cpp` activation family.

## Route Decision

- Selected next broader lifecycle route: broader translated `src/backend/riscv/codegen/atomics.cpp` activation centered on bringing the parked atomic owner surface across the shared RV64 header/build boundary and then narrowing to the first honest atomic owner packet.
- Narrowest honest continuation from the blocked checkpoint: one lifecycle rewrite that names atomics as the next broader family while keeping the first execution packet bounded to the minimum shared header/build/proof surface needed to make a first atomic entry path truthful.
- Parked families:
  - keep the stale `memory.cpp` typed-indirect / broader GEP route inactive rather than treating it as unfinished executor work
  - keep broader calls/comparison/variadic follow-on parked because the current route history already exhausted the nearby truthful continuations there
  - keep `inline_asm.cpp` parked until the branch needs more than the already-landed asm-emitter helper surface
  - keep `intrinsics.cpp` and `peephole.cpp` parked until the branch has a more direct shared-surface or native-emitter reason to consume them

## Recommendation

- Reviewer recommendation: `rewrite plan/todo before more execution`
- Rationale: the stale memory route is correctly blocked, and the nearest truthful continuation is not another memory-family reinterpretation. Canonical lifecycle should be rewritten to an atomics activation route before any executor packet is issued.
