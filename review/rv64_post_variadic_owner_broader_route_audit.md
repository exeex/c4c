# RV64 Post-Variadic Owner Broader Route Audit

- Review scope: `plan.md`, `todo.md`, `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`, `review/rv64_post_variadic_owner_route_audit.md`, `src/backend/riscv/codegen/variadic.cpp`, `src/backend/riscv/codegen/memory.cpp`, `src/backend/riscv/codegen/comparison.cpp`, `src/backend/riscv/codegen/calls.cpp`, `src/backend/riscv/codegen/alu.cpp`, `src/backend/riscv/codegen/globals.cpp`, and `git diff 1068e0d8..HEAD`
- Chosen review base: `1068e0d8` (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane after variadic owner seam`)
- Why this base is correct: `scripts/plan_change_gap.sh` reports `1068e0d8` as the latest canonical lifecycle checkpoint, and that commit is the plan/todo rewrite that accepted `067c5536` as the completed direct variadic owner-body seam while explicitly blocking the lane for a fresh broader-route decision. This audit is about choosing that next route from the blocked checkpoint itself, so the blocked lifecycle commit is the right base.
- Commits reviewed since base: `0`

## Findings

### 1. The branch is still truthfully parked at the blocked post-variadic-owner checkpoint, so any new executor packet requires a lifecycle rewrite first.

- `git diff 1068e0d8..HEAD` is empty and `git rev-list --count 1068e0d8..HEAD` is `0`, so there is no implementation drift to absorb before choosing the next route.
- Canonical state already says the direct `variadic.cpp` owner-body seam is complete and that the lane is blocked pending a fresh broader-route decision: [plan.md](/workspaces/c4c/plan.md:54), [plan.md](/workspaces/c4c/plan.md:63), [todo.md](/workspaces/c4c/todo.md:9), [todo.md](/workspaces/c4c/todo.md:16).
- The source idea now records the same blocked checkpoint after commit `067c5536`, so continuing straight into another executor packet from the current lifecycle state would be route drift rather than execution of an active packet: [ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md](/workspaces/c4c/ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md:480), [ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md](/workspaces/c4c/ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md:636).

### 2. The visible calls, comparison, variadic, globals, and current ALU lanes are exhausted at this route level, so they are not the narrowest honest continuation from this checkpoint.

- `variadic.cpp` now contains the three routed owner entries directly, and both canonical state and the prior review artifact treat that bounded seam as consumed. The remaining variadic follow-on is explicitly broader than the landed `emit_va_arg_impl(...)` / `emit_va_start_impl(...)` / `emit_va_copy_impl(...)` route: [src/backend/riscv/codegen/variadic.cpp](/workspaces/c4c/src/backend/riscv/codegen/variadic.cpp:65), [plan.md](/workspaces/c4c/plan.md:70), [todo.md](/workspaces/c4c/todo.md:43).
- `comparison.cpp` already carries the integer compare, fused compare-and-branch, select, and float-compare surfaces that the plan selected earlier, and the file header still marks broader comparison owner work as parked rather than exposing a smaller adjacent continuation: [src/backend/riscv/codegen/comparison.cpp](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:3), [src/backend/riscv/codegen/comparison.cpp](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:142), [src/backend/riscv/codegen/comparison.cpp](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:191).
- `calls.cpp` already includes the broader outgoing-call staging family plus the widened result-store path through `emit_call_store_result_impl(...)`; the remaining plan language keeps only broader calls-owner follow-on parked beyond that now-complete route: [src/backend/riscv/codegen/calls.cpp](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:92), [src/backend/riscv/codegen/calls.cpp](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:471), [plan.md](/workspaces/c4c/plan.md:76).
- `globals.cpp` and the current ALU owner surface are likewise exhausted for the active route family. TLS and global-address entry points are already real in `globals.cpp`, and the current ALU file only exposes the landed integer-owner surface rather than a smaller unclaimed neighbor: [src/backend/riscv/codegen/globals.cpp](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:48), [src/backend/riscv/codegen/globals.cpp](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:62), [src/backend/riscv/codegen/alu.cpp](/workspaces/c4c/src/backend/riscv/codegen/alu.cpp:2), [src/backend/riscv/codegen/alu.cpp](/workspaces/c4c/src/backend/riscv/codegen/alu.cpp:48).

### 3. The narrowest honest broader continuation now left on the board is the translated `memory.cpp` typed-indirect / broader GEP family.

- The active plan and todo already identify broader `memory.cpp` typed-indirect and broader GEP owner work as the principal parked memory family that remains after the variadic seam, while the other named families are exhausted or explicitly still parked: [plan.md](/workspaces/c4c/plan.md:70), [todo.md](/workspaces/c4c/todo.md:43), [ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md](/workspaces/c4c/ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md:649).
- `memory.cpp` is the only nearby owner file whose top-level comment still explicitly says the live slice stops short of the broader translated memory owner body, which matches the long-running parked family language in the idea: [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:2), [ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md](/workspaces/c4c/ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md:653).
- The live file already has the supporting address helpers that a broader memory continuation would consume: `resolve_slot_addr(...)`-based direct/indirect/over-aligned store/load lowering, pointer-load helpers, and the constant-address GEP helpers are real, but the route has never been rewritten to consume the still-parked typed-indirect / broader GEP owner work on top of them: [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:77), [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:112), [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:253), [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:263).
- Earlier route audits repeatedly kept this memory family parked because calls, globals, F128, and variadic were smaller or more truthful first. Those routes are now consumed, so keeping memory parked again would no longer reflect the nearest explicit remaining gap on the translated RV64 surface: [review/rv64_post_calls_result_store_wide_broader_route_audit.md](/workspaces/c4c/review/rv64_post_calls_result_store_wide_broader_route_audit.md:28), [review/rv64_post_aggregate_outgoing_call_broader_route_audit.md](/workspaces/c4c/review/rv64_post_aggregate_outgoing_call_broader_route_audit.md:26).

## Judgments

- Plan-alignment judgment: `on track`
- Technical-debt judgment: `action needed`
- Route judgment: the current blocked checkpoint is still correct, and the next truthful broader lifecycle route is no longer in variadic, calls, or comparison; it is the broader translated `memory.cpp` typed-indirect / broader GEP family.

## Route Decision

- Selected next broader lifecycle route: broader translated `src/backend/riscv/codegen/memory.cpp` follow-on centered on typed-indirect memory ownership plus the broader GEP owner work that remains parked after the landed helper/default-load-store seams.
- Narrowest honest continuation from the blocked checkpoint: one lifecycle rewrite that names a broader shared RV64 memory route built on the already-landed `SlotAddr` / `resolve_slot_addr(...)` / pointer-address helper surface, widening only as needed to make the translated typed-indirect / broader GEP owner work truthful with a matching focused proof lane.
- Parked families:
  - keep broader variadic follow-on beyond the landed `emit_va_arg_impl(...)` / `emit_va_start_impl(...)` / `emit_va_copy_impl(...)` seam parked
  - keep broader calls-owner follow-on beyond the now-complete widened result-store route parked
  - keep broader comparison follow-on beyond the landed `emit_fused_cmp_branch_impl(...)` / `emit_select_impl(...)` route parked
  - keep the exhausted globals lane and the exhausted current ALU integer-owner lane parked rather than reopening them as another direct packet

## Recommendation

- Reviewer recommendation: `rewrite plan/todo before more execution`
- Rationale: the branch is aligned with the blocked post-variadic-owner checkpoint, but the route is consumed and canonical state must move before any further execution. The nearest truthful continuation is now the broader memory typed-indirect / broader GEP family, not another packet inside variadic, calls, comparison, globals, or the current ALU surface.
