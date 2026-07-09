# Review C: Idea 645 Step 3 Route Quality

Active source idea: `ideas/open/645_rv64_branch_residual_terminator_fragment_lowering.md`

Chosen review base: `879e7114b [todo_only] Select RV64 fused pointer branch target`

Base rationale: `c0b54f3b` activated idea 645, but `879e7114b` is the latest lifecycle checkpoint that selected the exact Step 3 target under review: a prepared fused pointer compare branch whose fused condition and exactly one pointer operand are stack-backed with explicit `branch_stack_load_authority`. The current implementation is an uncommitted worktree slice on top of that checkpoint.

Commit count since base: `0` committed implementation commits, plus the current uncommitted Step 3 worktree diff.

## Findings

### Low: symmetric condition-plus-LHS positive coverage is not explicit

The implementation admits either one stack-backed pointer operand when the fused condition is stack-backed: `selected_condition_and_single_operand_stack_branch_freshness_allows_pointer_publication()` rejects both-stack and neither-stack cases, then accepts the LHS-stack or RHS-stack side by checking the opposite operand for register/null support (`src/backend/mir/riscv/codegen/object_emission.cpp:12285`, `src/backend/mir/riscv/codegen/object_emission.cpp:12306`). The new focused positive fixture covers condition-plus-RHS-stack emission (`tests/backend/mir/backend_riscv_object_emission_test.cpp:958`, `tests/backend/mir/backend_riscv_object_emission_test.cpp:15510`). Existing LHS/RHS authority negatives still cover operand authority status, and the selected Step 3 proof row is RHS-stack, so this is not blocking. Step 4 should add either a condition-plus-LHS mirror positive or explicitly record that only the RHS-selected representative is being accepted for this packet.

## Route Review

The diff matches idea 645 and the Step 3 target. It stays in RV64 object-route terminator lowering and does not publish branch freshness or clobber-safety authority. The new condition authority path calls the existing selected branch-stack-load authority checker with `PreparedBranchStackLoadRole::Condition` (`src/backend/mir/riscv/codegen/object_emission.cpp:12143`) and uses that as admission before fused pointer branch publication (`src/backend/mir/riscv/codegen/object_emission.cpp:12393`). Operand authority remains routed through the existing LHS/RHS checks (`src/backend/mir/riscv/codegen/object_emission.cpp:12403`, `src/backend/mir/riscv/codegen/object_emission.cpp:12413`).

I do not see acceptance by stack offset, frame home, final assembly shape, or assumed no-clobber behavior in the implementation path. Stack homes are used to decide when authority is required and to materialize the already-authorized value through the normal value movement path (`src/backend/mir/riscv/codegen/object_emission.cpp:12455`), not as a substitute for authority. The condition value is not reloaded as a bool; the test checks that condition authority is admission-only while the branch compares the pointer operands (`tests/backend/mir/backend_riscv_object_emission_test.cpp:15530`).

The slice is shape-based rather than named-case overfit. I found no `src/20140828-1.c` gate, no expectation downgrade, no allowlist/accounting change in code or tests, and no new source-file-specific dispatch. The positive fixture constructs a prepared fused pointer branch shape, and the negative diagnostics mutate prepared authority records rather than matching the representative row.

The new runtime mismatch is acceptable for this slice. The recorded proof shows build success, 8/8 focused CTests passing, and the selected row advancing past `unsupported_terminator_fragment` before failing later at `RV64_BACKEND_RUNTIME_MISMATCH`. That satisfies the Step 3 completion check as semantic terminator-lowering progress, provided the runtime residual remains Step 4 classification work and is not claimed as closure.

## Judgments

- Idea alignment: `matches source idea`
- Runbook transcription: `plan matches idea`
- Route alignment: `on track`
- Technical debt: `watch`
- Validation sufficiency: `narrow proof sufficient`
- Reviewer recommendation: `continue current route`

