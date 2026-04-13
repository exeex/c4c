# RV64 Step 27 Memory Route Sanity Audit

- Review scope: `plan.md`, `todo.md`, `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`, `review/rv64_post_variadic_owner_broader_route_audit.md`, `src/backend/riscv/codegen/memory.cpp`, `tests/backend/backend_shared_util_tests.cpp`, and git history touching the RV64 memory route
- Chosen review base: `156e29f2` (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to broader memory typed-indirect GEP family`)
- Why this base is correct: `scripts/plan_change_gap.sh` reports `156e29f2` as the latest canonical lifecycle checkpoint, and that commit is exactly the Step 27 rewrite now under sanity check. The question is whether the newly activated route is still truthful after that rewrite, so the route-activation commit itself is the right review base.
- Commits reviewed since base: `0`

## Findings

### 1. The active Step 27 route appears to have been activated onto materially landed memory work rather than an unfinished executor surface.

- `memory.cpp` already contains live implementations for the exact typed-indirect/default-load-store and constant-GEP-facing entry points that the new route describes: `emit_store_impl(...)`, `emit_load_impl(...)`, `emit_store_with_const_offset_impl(...)`, `emit_load_with_const_offset_impl(...)`, `emit_slot_addr_to_secondary_impl(...)`, `emit_gep_direct_const_impl(...)`, and `emit_gep_indirect_const_impl(...)`: [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:59), [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:77), [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:262), [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:277).
- Focused shared-util coverage for those seams already exists in-tree. The current test file exercises pointer-resolution, constant-GEP helpers, and the scalar memory owner path with direct, indirect, and over-aligned address resolution: [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:891), [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1050).
- Git history for `memory.cpp` already records the relevant landing sequence: helper seam (`fed0d66e`), pointer-resolution (`7db8106e`), GEP constant helpers (`8a15ac5c`), shared slot-address surface (`e12a8092`), scalar memory owner paths (`88a82dbe`), and shared F128 memory seam (`d8a09656`): `git log --oneline -- src/backend/riscv/codegen/memory.cpp`.

### 2. The current Step 27 route selection leans on stale route language more than on the live implementation and test surface.

- The main positive evidence in `review/rv64_post_variadic_owner_broader_route_audit.md` is the lingering top-of-file comment in `memory.cpp` saying the live slice stops short of the broader memory owner body: [review/rv64_post_variadic_owner_broader_route_audit.md](/workspaces/c4c/review/rv64_post_variadic_owner_broader_route_audit.md:25), [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:2).
- That comment is weaker than the live code and tests immediately below it. The same review artifact cites the existing live store/load and GEP helper entry points as evidence for a future route, but those seams are not parked declarations; they are already compiled and covered: [review/rv64_post_variadic_owner_broader_route_audit.md](/workspaces/c4c/review/rv64_post_variadic_owner_broader_route_audit.md:27), [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:77), [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:112), [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1059).
- The older owner-file audit after the scalar default-load/store packet said the remaining visible memory follow-on widened into typed-indirect access, broader GEP owner bodies, or wider shared RV64 state, and therefore was not another bounded continuation at that checkpoint: [review/rv64_post_default_load_store_owner_file_audit.md](/workspaces/c4c/review/rv64_post_default_load_store_owner_file_audit.md:19). The new Step 27 rewrite does not identify a fresh narrower sub-packet inside that broader family; it promotes the broader family itself to active execution state.

### 3. Because there is no new implementation delta past `156e29f2`, issuing an executor packet from the current lifecycle state would risk route drift.

- `git diff 156e29f2..HEAD` is empty and `git rev-list --count 156e29f2..HEAD` is `0`, so there is no newly landed memory implementation to reinterpret or absorb.
- The repo is therefore evaluating the active route selection itself, not any fresh code. If the selected Step 27 family is already materially landed, the right next action is another lifecycle correction, not execution from a stale or overstated route.

## Judgments

- Plan-alignment judgment: `drifting`
- Technical-debt judgment: `action needed`
- Route judgment: the active Step 27 memory typed-indirect / broader GEP route does not look like a truthful unfinished executor lane. It appears to have routed onto seams that are already materially live in `memory.cpp` with existing shared-util coverage, so lifecycle should be rewritten again before any executor packet is issued.

## Recommendation

- Reviewer recommendation: `rewrite plan/todo before more execution`
- Rationale: the current route-selection commit `156e29f2` appears to overstate how much of the RV64 memory typed-indirect / broader GEP family remains unlanded. The supervisor should not issue a Step 27 executor packet from this state. Canonical lifecycle needs another route decision or block checkpoint that accounts for the already-landed `memory.cpp` implementation and tests instead of treating that surface as newly active work.
