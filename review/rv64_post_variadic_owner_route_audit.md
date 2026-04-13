# RV64 Post-Variadic Owner Route Audit

- Review scope: `plan.md`, `todo.md`, `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`, `src/backend/riscv/codegen/variadic.cpp`, `tests/backend/backend_shared_util_tests.cpp`, `CMakeLists.txt`, and `git diff 5838fa76..HEAD`
- Chosen review base: `5838fa76` (`[plan_change] [todo_change] [idea_open_change] lifecycle: advance rv64 variadic lane past header prologue slice`)
- Why this base is correct: `scripts/plan_change_gap.sh` reports `5838fa76` as the latest canonical lifecycle checkpoint, and that commit is the plan/todo rewrite that advanced the active route past the header/prologue packet to the direct `variadic.cpp` owner-body seam now under review. This audit is about whether that exact active packet is now complete and whether canonical lifecycle must move again before more execution.
- Commits reviewed since base: `1`

## Findings

### 1. The landed implementation completes the active Step 26 packet cleanly, so canonical lifecycle is now behind `HEAD`.

- `plan.md` still marks Step 26 as `Status: active` and defines the route as the direct translated `variadic.cpp` owner-body seam centered on `emit_va_arg_impl(...)`, `emit_va_start_impl(...)`, and `emit_va_copy_impl(...)`: [plan.md](/workspaces/c4c/plan.md:1220).
- `todo.md` still says the next active bounded packet is the direct `variadic.cpp` owner-body seam after commit `78bc9ae1`: [todo.md](/workspaces/c4c/todo.md:9).
- `HEAD` now lands exactly that packet. `src/backend/riscv/codegen/variadic.cpp` implements the three owner bodies through the already-landed variadic state and prologue setup surface, including bounded pointer resolution for allocas/register-backed values plus the scalar and `F128` `va_arg` paths: [src/backend/riscv/codegen/variadic.cpp](/workspaces/c4c/src/backend/riscv/codegen/variadic.cpp:65).
- The route also includes only the minimum build/proof wiring needed to make the seam real: `variadic.cpp` is added to the RV64 source lists in [`CMakeLists.txt`](/workspaces/c4c/CMakeLists.txt:234) and [`CMakeLists.txt`](/workspaces/c4c/CMakeLists.txt:473), and focused emitted-text coverage lands in [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:2808).

### 2. The diff stays within the active route boundary and does not show route drift into broader parked families.

- The full diff since the lifecycle checkpoint touches only `CMakeLists.txt`, `src/backend/riscv/codegen/variadic.cpp`, and `tests/backend/backend_shared_util_tests.cpp`, which matches the Step 26 target/proof surface: `git diff --stat 5838fa76..HEAD`.
- The implementation does not reopen broader `memory.cpp`, `comparison.cpp`, or `calls.cpp` follow-on work. The new helper logic in `variadic.cpp` is narrowly scoped to address resolution plus the three owner entries rather than expanding shared RV64 state again: [src/backend/riscv/codegen/variadic.cpp](/workspaces/c4c/src/backend/riscv/codegen/variadic.cpp:30).
- The test addition is likewise route-shaped rather than speculative. It proves `va_start`, `va_copy`, scalar `va_arg`, and the bounded `F128` truncation path without widening into a larger runtime or ABI-validation family: [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:2808).

### 3. More execution should not be issued from the current lifecycle state because the active packet appears exhausted and there is no fresh canonical route after it.

- `plan.md` ends at Step 26, and that step's completion checks are now satisfied by the landed seam plus focused proof coverage: [plan.md](/workspaces/c4c/plan.md:1250).
- `todo.md` still exposes the just-landed packet as the next executor work item, so issuing another executor packet from the current lifecycle state would stack work on top of stale canonical guidance: [todo.md](/workspaces/c4c/todo.md:17).
- The source idea and plan guardrails still explicitly park broader variadic follow-on beyond the bounded `emit_va_arg_impl(...)` / `emit_va_start_impl(...)` / `emit_va_copy_impl(...)` seam, so the next truthful action is a lifecycle decision, not ad hoc continuation into adjacent variadic or memory work: [plan.md](/workspaces/c4c/plan.md:1243), [todo.md](/workspaces/c4c/todo.md:46).

## Judgments

- Plan-alignment judgment: `on track`
- Technical-debt judgment: `acceptable`
- Route judgment: commit `067c5536` completes the active direct RV64 variadic owner-body seam cleanly and without visible drift, but the route is now consumed; canonical lifecycle must advance before any further executor packet is issued.

## Recommendation

- Reviewer recommendation: `rewrite plan/todo before more execution`
- Rationale: the branch is aligned with the active runbook and the newly landed variadic owner-body slice appears complete, but `plan.md` and `todo.md` still describe that slice as pending. The supervisor should hand this checkpoint to the plan owner so canonical lifecycle can record `067c5536` as the completed Step 26 packet and decide whether the lane is now blocked pending a fresh broader-route decision or has a newly selected next route.
