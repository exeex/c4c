# RV64 Post-Calls Result-Store Wide Route Audit

- Review scope: `plan.md`, `todo.md`, `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`, `review/rv64_post_aggregate_outgoing_call_broader_route_audit.md`, `src/backend/riscv/codegen/calls.cpp`, `tests/backend/backend_shared_util_tests.cpp`, and `git diff 1f8e7761..HEAD`
- Chosen review base: `1f8e7761` (`[plan_change] [todo_change] [idea_open_change] lifecycle: retarget rv64 calls result-store route`)
- Why this base is correct: `scripts/plan_change_gap.sh` reports `1f8e7761` as the last canonical lifecycle checkpoint, and `git log --oneline --grep='\[plan_change\]' -- plan.md todo.md ideas/open/` shows it is the commit that activated the current broader `calls.cpp::emit_call_store_result_impl(...)` route after the blocked post-aggregate checkpoint. The current review question is whether execution since that retarget has completed that exact route, so the retarget commit itself is the right base.
- Commits reviewed since base: `1`

## Findings

### 1. The active lifecycle route is now exhausted, so issuing another executor packet without a rewrite would drift past the packet boundary.

- The current lifecycle still says the active executor work is the parked `I128` / `F128` continuation inside `calls.cpp::emit_call_store_result_impl(...)`: [plan.md](/workspaces/c4c/plan.md:62), [plan.md](/workspaces/c4c/plan.md:77), [todo.md](/workspaces/c4c/todo.md:9), [todo.md](/workspaces/c4c/todo.md:29).
- The only commit after that retarget is `873707a0`, so `git diff 1f8e7761..HEAD` contains no extra route widening beyond the selected packet.
- `review/rv64_post_aggregate_outgoing_call_broader_route_audit.md` defined the narrowest honest continuation as one bounded packet that makes the `I128` paired-result store real and wires the `F128` paired store plus truncate-to-accumulator handoff through the existing helper surface: [review/rv64_post_aggregate_outgoing_call_broader_route_audit.md](/workspaces/c4c/review/rv64_post_aggregate_outgoing_call_broader_route_audit.md:37).
- `873707a0` lands exactly that packet and nothing broader, so there is not another bounded executor packet left inside the same active route.

### 2. Commit `873707a0` truthfully completes the broader `emit_call_store_result_impl(...)` `I128` / `F128` route selected by the previous audit.

- `emit_call_store_result_impl(...)` no longer returns early for `I128` and `F128`; it now emits paired `a0`/`a1` slot stores for `I128` and calls `f128_store_result_and_truncate(dest)` for `F128`: [src/backend/riscv/codegen/calls.cpp](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:471).
- That matches the translated Rust owner shape the earlier audit cited as the truthful continuation for the parked wide-result cases: `calls.rs` stores `a0`/`a1` for `I128`, and for long double stores both halves before `__trunctfdf2` plus accumulator materialization: `ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:457`.
- The existing RV64 helper surface already provided the required support, so the commit did not silently expand into a new family: `emit_i128_store_result_impl(...)` already existed for paired-result storage and `f128_store_result_and_truncate(...)` already owned the store-plus-truncate behavior through the landed soft-float seam: [src/backend/riscv/codegen/f128.cpp](/workspaces/c4c/src/backend/riscv/codegen/f128.cpp:222), [src/backend/riscv/codegen/i128_ops.cpp](/workspaces/c4c/src/backend/riscv/codegen/i128_ops.cpp:374).
- The earlier idea text also frames this exact parked `I128` / `F128` continuation as the next truthful lifecycle route after the aggregate family finished, not as a multi-packet family under the same active state: [ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md](/workspaces/c4c/ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md:451).

### 3. The new proof is still narrow, but it is aligned with the selected route rather than evidence of unfinished work inside it.

- The diff adds focused shared-util coverage for the exact wide-result cases the previous route audit identified as missing: `I128` slot stores, `F128` slot preservation, `__trunctfdf2`, and register materialization after truncation: [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1314).
- This remains a bounded text-shape proof rather than a broader regression-guard run, so the proof bar is still `watch` rather than fully de-risked integration coverage.
- That narrower proof does not imply another executor packet inside the current route; it only means the next lifecycle step should choose the next broader route explicitly instead of silently continuing.

## Judgments

- Plan-alignment judgment: `on track`
- Technical-debt judgment: `watch`
- Route judgment: commit `873707a0` truthfully completed the current broader `calls.cpp::emit_call_store_result_impl(...)` `I128` / `F128` route, and no second bounded executor packet remains inside that same active route.

## Recommendation

- Reviewer recommendation: `rewrite plan/todo before more execution`
- Rationale: the implementation since `1f8e7761` matches the selected route and exhausts it. Canonical lifecycle now lags behind the code, so the supervisor should hand this report to `c4c-plan-owner` for a new checkpoint rather than issuing another executor packet on the assumption that the current calls result-store route is still open.
