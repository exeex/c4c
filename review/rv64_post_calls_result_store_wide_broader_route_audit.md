# RV64 Post-Calls Result-Store Wide Broader Route Audit

- Review scope: `plan.md`, `todo.md`, `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`, `review/rv64_post_calls_result_store_wide_route_audit.md`, `src/backend/riscv/codegen/variadic.cpp`, `src/backend/riscv/codegen/prologue.cpp`, `src/backend/riscv/codegen/riscv_codegen.hpp`, `src/backend/riscv/codegen/calls.cpp`, `src/backend/riscv/codegen/memory.cpp`, `src/backend/riscv/codegen/comparison.cpp`, `ref/claudes-c-compiler/src/backend/riscv/codegen/variadic.rs`, `ref/claudes-c-compiler/src/backend/riscv/codegen/prologue.rs`, and `git diff cece6cb3..HEAD`
- Chosen review base: `cece6cb3` (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane after calls result-store wide route`)
- Why this base is correct: `scripts/plan_change_gap.sh` reports `cece6cb3` as the latest canonical lifecycle checkpoint, and `git log --oneline --grep='\[plan_change\]' -- plan.md todo.md ideas/open/` shows it is the commit that accepted `873707a0` as the landed wide calls-result route while explicitly blocking the lane for a fresh broader-route decision. This review is about choosing that next route from the blocked checkpoint itself, so the block commit is the right base.
- Commits reviewed since base: `0`

## Findings

### 1. The branch is still truthfully parked at the blocked checkpoint, so any new executor packet requires a lifecycle rewrite first.

- `git diff cece6cb3..HEAD` is empty and `git rev-list --count cece6cb3..HEAD` is `0`, so there is no implementation drift to absorb before choosing the next route.
- Canonical state already says the wider `calls.cpp::emit_call_store_result_impl(...)` route is complete and that the lane is blocked pending a fresh broader-route decision: [plan.md](/workspaces/c4c/plan.md:62), [plan.md](/workspaces/c4c/plan.md:69), [todo.md](/workspaces/c4c/todo.md:9), [todo.md](/workspaces/c4c/todo.md:23).
- The accepted prior audit also says no second bounded executor packet remains inside the just-finished calls-result route: [review/rv64_post_calls_result_store_wide_route_audit.md](/workspaces/c4c/review/rv64_post_calls_result_store_wide_route_audit.md:10).

### 2. The narrowest honest continuation from this checkpoint is a broader RV64 variadic activation route, not another isolated calls, memory, or comparison packet.

- `variadic.cpp` is still only a helper mirror. It exposes local arithmetic helpers plus commented future method boundaries, but no real owner entries yet: [src/backend/riscv/codegen/variadic.cpp](/workspaces/c4c/src/backend/riscv/codegen/variadic.cpp:30).
- The shared RV64 header does not currently expose the variadic owner methods or the state needed to make them truthful. `RiscvCodegenState` has slots/register/GOT/alloca tracking only, and `RiscvCodegen` declares no `emit_va_arg_impl(...)`, `emit_va_start_impl(...)`, or `emit_va_copy_impl(...)`: [src/backend/riscv/codegen/riscv_codegen.hpp](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:318), [src/backend/riscv/codegen/riscv_codegen.hpp](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:510).
- `prologue.cpp` explicitly says the live C++ seam stops at return-state / return-exit work and still parks broader stack sizing and parameter storage, which is exactly the missing bridge for a truthful variadic lane: [src/backend/riscv/codegen/prologue.cpp](/workspaces/c4c/src/backend/riscv/codegen/prologue.cpp:2), [src/backend/riscv/codegen/prologue.cpp](/workspaces/c4c/src/backend/riscv/codegen/prologue.cpp:16).
- The translated Rust reference shows the minimum broader route needed to make variadics real: `prologue.rs` tracks `va_named_gp_count`, `va_named_stack_bytes`, and variadic-frame state before/alongside prologue setup, while `variadic.rs` uses exactly that state plus the existing slot/register helpers to implement `emit_va_arg_impl(...)`, `emit_va_start_impl(...)`, and `emit_va_copy_impl(...)`: `ref/claudes-c-compiler/src/backend/riscv/codegen/prologue.rs:17`, `ref/claudes-c-compiler/src/backend/riscv/codegen/prologue.rs:115`, `ref/claudes-c-compiler/src/backend/riscv/codegen/variadic.rs:8`.
- That makes the truthful next route a bounded shared-surface expansion centered on RV64 variadic state and prologue ownership, with `variadic.cpp` as the consumer file, rather than pretending `variadic.cpp` alone is already a standalone one-method packet.

### 3. The other visible parked families should stay parked because they are either exhausted at the current route level or broader than the variadic activation seam.

- `calls.cpp::emit_call_store_result_impl(...)` already covers the previously parked `I128` and `F128` paths, and the accepted checkpoint says that route is exhausted: [src/backend/riscv/codegen/calls.cpp](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:471), [review/rv64_post_calls_result_store_wide_route_audit.md](/workspaces/c4c/review/rv64_post_calls_result_store_wide_route_audit.md:17).
- `comparison.cpp` already landed the broader control-flow seam through `emit_fused_cmp_branch_impl(...)` and `emit_select_impl(...)`, so any next comparison move is broader follow-on work beyond the accepted route boundary: [src/backend/riscv/codegen/comparison.cpp](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:191), [src/backend/riscv/codegen/comparison.cpp](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:210).
- `memory.cpp` already exposes the helper/direct seams currently named in canonical state, and the remaining memory work is still the broader typed-indirect / broader GEP follow-on that the plan keeps parked: [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:262), [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:277), [plan.md](/workspaces/c4c/plan.md:68).

## Judgments

- Plan-alignment judgment: `on track`
- Technical-debt judgment: `action needed`
- Route judgment: the lane is correctly blocked at `cece6cb3`, and the next truthful lifecycle route is a broader RV64 variadic activation family centered on shared variadic state plus prologue-owned setup, not another direct packet inside the exhausted calls route.

## Route Decision

- Selected next broader lifecycle route: broader translated RV64 variadic activation centered on
  `src/backend/riscv/codegen/riscv_codegen.hpp` variadic state/method declarations,
  `src/backend/riscv/codegen/prologue.cpp` variadic register-save-area / named-argument tracking,
  and `src/backend/riscv/codegen/variadic.cpp` owner bodies.
- Narrowest honest continuation from the blocked checkpoint: one lifecycle rewrite that names a broader shared-surface packet for `va_named_gp_count`, `va_named_stack_bytes`, variadic-frame setup, and the translated `emit_va_arg_impl(...)`, `emit_va_start_impl(...)`, and `emit_va_copy_impl(...)` seam, with build wiring only as needed to make that surface real.
- Parked families:
  - keep broader `memory.cpp` typed-indirect and broader GEP owner work parked
  - keep broader comparison follow-on beyond `emit_fused_cmp_branch_impl(...)` / `emit_select_impl(...)` parked
  - keep any broader calls-owner follow-on beyond the now-complete wide result-store route parked
  - do not reopen the exhausted calls result-store route as another direct packet

## Recommendation

- Reviewer recommendation: `rewrite plan/todo before more execution`
- Rationale: the repo is aligned with the blocked post-calls-result-store-wide checkpoint, but the next honest step is no longer another narrow owner-body packet on the existing seam. Canonical lifecycle should be rewritten to a broader variadic/header/prologue route before any executor packet is issued.
